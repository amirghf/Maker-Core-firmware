// اجرای دستورهای حرکت G0 و G1 (بند ۵.۱۱). فقط منطق دستورها و تولید بلوک‌ها.
#include "motion_cmd.h"
#include "machine_state.h"
#include "motion_modes.h"
#include "motion_queue.h"
#include "motion_limits.h"
#include "soft_limits.h"
#include "extruder.h"
#include "stepper_out.h"
#include "axis_move.h"
#include "hal_step_timer.h"
#include "error.h"
#include <math.h>
#include <stdlib.h>

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند. شماره‌ها دائمی‌اند.
extern "C" {
volatile uint32_t mc_dbg_164_gm_result   = 0;  // خواندنی: نتیجه آخرین G0/G1 (0 هیچ، 1 پذیرفته، 2 پارامتر نامعتبر، 3 حد نرم‌افزاری، 4 kill یا arm نیست، 5 اکستروژن سرد، 6 خطا وسط ورود، 7 حرکت بیش از حد بزرگ)
volatile uint32_t mc_dbg_165_gm_accepted = 0;  // خواندنی: تعداد G0/G1 پذیرفته‌شده
volatile uint32_t mc_dbg_166_gm_refused  = 0;  // خواندنی: تعداد G0/G1 ردشده
volatile uint32_t mc_dbg_167_gm_segments = 0;  // خواندنی: تعداد کل بلوک‌های واردشده به صف
volatile uint32_t mc_dbg_168_gm_clamped  = 0;  // خواندنی: تعداد حرکت‌های کوتاه/محدودشده در سرعت یا شتاب
volatile int32_t  mc_dbg_169_gm_pos_x_um = 0;  // خواندنی: موقعیت منطقی X به میکرومتر
volatile int32_t  mc_dbg_170_gm_pos_y_um = 0;  // Y
volatile int32_t  mc_dbg_171_gm_pos_z_um = 0;  // Z
volatile int32_t  mc_dbg_172_gm_pos_e_um = 0;  // E
volatile uint32_t mc_dbg_182_mt_en_mask  = 0;  // خواندنی: درایورهای فعال (بیت ۰ = X ... بیت ۳ = E)
volatile uint32_t mc_dbg_183_mt_refused  = 0;  // خواندنی: تعداد M17/M18/M84 ردشده
extern volatile uint32_t mc_dbg_80_mv_arm;
extern volatile uint32_t mc_dbg_112_mq_run;
}

namespace mc {
namespace {

struct MoveStateCtx {
    int32_t  total_delta[4];
    uint32_t max_delta;
    uint32_t num_blocks;
    uint32_t current_block_k;
    float    start_pos[4];
    float    target_pos[4];
    float    spmm[4];
    int32_t  queued_steps[4];
    uint32_t start_hz;
    uint32_t cruise_hz;
    uint32_t accel_sps2;
    uint8_t  dir_mask;
};

MoveStateCtx g_move = {};

void record_refuse(uint32_t result_code) {
    mc_dbg_164_gm_result = result_code;
    mc_dbg_166_gm_refused += 1U;
}

int32_t to_um(float mm) {
    return lroundf(mm * 1000.0f);
}

}  // namespace

void mc_gmove_init() {
    g_move = MoveStateCtx{};
    mc_dbg_164_gm_result   = 0U;
    mc_dbg_165_gm_accepted = 0U;
    mc_dbg_166_gm_refused  = 0U;
    mc_dbg_167_gm_segments = 0U;
    mc_dbg_168_gm_clamped  = 0U;
    mc_dbg_182_mt_en_mask  = 0U;
    mc_dbg_183_mt_refused  = 0U;
}

void mc_gmove_poll() {
    mc_dbg_169_gm_pos_x_um = to_um(mc_position_get(0U));
    mc_dbg_170_gm_pos_y_um = to_um(mc_position_get(1U));
    mc_dbg_171_gm_pos_z_um = to_um(mc_position_get(2U));
    mc_dbg_172_gm_pos_e_um = to_um(mc_position_get(3U));

    uint32_t mask = 0U;
    for (uint8_t i = 0U; i < 4U; ++i) {
        if (mc_stepper_is_enabled(static_cast<StepAxis>(i))) {
            mask |= (1U << i);
        }
    }
    mc_dbg_182_mt_en_mask = mask;
}

void mc_gmove_note_mt_refused() {
    mc_dbg_183_mt_refused += 1U;
}

GcodeStatus mc_gmove_run(const GcodeCommand& cmd, bool first_call) {
    if (first_call) {
        // الف) F parameter
        if (mc_gcode_has(cmd, 'F')) {
            if (!mc_gcode_has_value(cmd, 'F')) {
                record_refuse(2U);
                return GcodeStatus::Error;
            }
            const float f_val = mc_gcode_value(cmd, 'F', 0.0f);
            if (!mc_mode_feed_set(f_val)) {
                record_refuse(2U);
                return GcodeStatus::Error;
            }
        }

        // ب) X, Y, Z, E parameters check
        static const char kLetters[4] = { 'X', 'Y', 'Z', 'E' };
        float raw_val[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        bool  has_axis[4] = { false, false, false, false };

        for (uint8_t i = 0U; i < 4U; ++i) {
            has_axis[i] = mc_gcode_has(cmd, kLetters[i]);
            if (has_axis[i]) {
                if (!mc_gcode_has_value(cmd, kLetters[i])) {
                    record_refuse(2U);
                    return GcodeStatus::Error;
                }
                raw_val[i] = mc_gcode_value(cmd, kLetters[i], 0.0f);
                if (isnan(raw_val[i]) || (fabsf(raw_val[i]) > kModeMaxPositionMm)) {
                    record_refuse(2U);
                    return GcodeStatus::Error;
                }
            }
        }

        // ج) Target position calculation
        float current_pos[4];
        float target_pos[4];
        const bool rel_xyz = mc_mode_relative_get();
        const bool rel_e   = mc_mode_e_relative_get();

        for (uint8_t i = 0U; i < 4U; ++i) {
            current_pos[i] = mc_position_get(i);
            if (has_axis[i]) {
                const bool is_rel = (i == 3U) ? rel_e : rel_xyz;
                target_pos[i] = is_rel ? (current_pos[i] + raw_val[i]) : raw_val[i];
            } else {
                target_pos[i] = current_pos[i];
            }
        }

        // د) Safety condition
        if (mc_is_killed() || (mc_dbg_80_mv_arm != 1U)) {
            record_refuse(4U);
            return GcodeStatus::Error;
        }

        // هـ) Soft limits check
        if (!mc_softlimit_allows_all(target_pos)) {
            mc_softlimit_note_refused();
            record_refuse(3U);
            return GcodeStatus::Error;
        }

        // و) Delta steps and dir mask
        int32_t delta[4];
        float   spmm[4];
        uint8_t dir_mask = 0U;
        uint32_t max_delta = 0U;
        bool any_steps = false;

        for (uint8_t i = 0U; i < 4U; ++i) {
            spmm[i] = mc_mode_spmm_get(i);
            const int32_t target_s = lroundf(target_pos[i] * spmm[i]);
            const int32_t pos_s    = lroundf(current_pos[i] * spmm[i]);
            delta[i] = target_s - pos_s;

            if (labs(delta[i]) > 100000000L) {
                record_refuse(7U);
                return GcodeStatus::Error;
            }

            if (delta[i] != 0) {
                any_steps = true;
                if (delta[i] > 0) {
                    dir_mask |= static_cast<uint8_t>(1U << i);
                }
                const uint32_t abs_d = static_cast<uint32_t>(labs(delta[i]));
                if (abs_d > max_delta) {
                    max_delta = abs_d;
                }
            }
        }

        // ز) If all delta == 0
        if (!any_steps) {
            for (uint8_t i = 0U; i < 4U; ++i) {
                mc_position_set(i, target_pos[i]);
            }
            mc_dbg_164_gm_result   = 1U;
            mc_dbg_165_gm_accepted += 1U;
            return GcodeStatus::Done;
        }

        // ح) Cold extrusion check
        if ((delta[3] != 0) && !mc_extruder_allowed()) {
            mc_extruder_note_refused();
            record_refuse(5U);
            return GcodeStatus::Error;
        }

        // ط) Speed and Acceleration calculations
        const bool has_xyz = (delta[0] != 0) || (delta[1] != 0) || (delta[2] != 0);
        float len_mm = 0.0f;

        if (has_xyz) {
            float len2 = 0.0f;
            for (uint8_t i = 0U; i < 3U; ++i) {
                const float mm_i = static_cast<float>(labs(delta[i])) / spmm[i];
                len2 += mm_i * mm_i;
            }
            len_mm = sqrtf(len2);
        } else {
            len_mm = static_cast<float>(labs(delta[3])) / spmm[3];
        }

        uint8_t master = 0U;
        for (uint8_t i = 1U; i < 4U; ++i) {
            if (labs(delta[i]) > labs(delta[master])) {
                master = i;
            }
        }

        const float hz_per_mms = static_cast<float>(labs(delta[master])) / len_mm;
        float v_mms = mc_mode_feed_get_mms();
        bool clamped = false;

        if (has_xyz) {
            for (uint8_t i = 0U; i < 4U; ++i) {
                if (delta[i] != 0) {
                    const float axis_mm = static_cast<float>(labs(delta[i])) / spmm[i];
                    const float max_axis_v = mc_mlim_feed_get(i);
                    const float max_v = max_axis_v * (len_mm / axis_mm);
                    if (v_mms > max_v) {
                        v_mms = max_v;
                        clamped = true;
                    }
                }
            }
        } else {
            const float max_e_v = mc_mlim_feed_get(3U);
            if (v_mms > max_e_v) {
                v_mms = max_e_v;
                clamped = true;
            }
        }

        uint32_t cruise_hz = static_cast<uint32_t>(lroundf(v_mms * hz_per_mms));
        if (cruise_hz > kAxisMoveMaxRateHz) {
            cruise_hz = kAxisMoveMaxRateHz;
            clamped = true;
        }
        if (cruise_hz < 1U) {
            cruise_hz = 1U;
        }

        const float min_feed = mc_mlim_minfeed_get();
        uint32_t start_hz = static_cast<uint32_t>(lroundf(min_feed * hz_per_mms));
        if (start_hz < kStepTimerMinHz) {
            start_hz = kStepTimerMinHz;
        }
        if (start_hz > cruise_hz) {
            start_hz = cruise_hz;
        }

        uint8_t kind = 2U;
        if (!has_xyz && (delta[3] != 0)) {
            kind = 1U;
        } else if (has_xyz && (delta[3] > 0)) {
            kind = 0U;
        }

        const float acc_mms2 = mc_mlim_acc_get(kind);
        uint32_t accel_sps2 = static_cast<uint32_t>(lroundf(acc_mms2 * hz_per_mms));
        if (accel_sps2 < kAxisAccelMinSps2) {
            accel_sps2 = kAxisAccelMinSps2;
            clamped = true;
        } else if (accel_sps2 > kAxisAccelMaxSps2) {
            accel_sps2 = kAxisAccelMaxSps2;
            clamped = true;
        }

        if (clamped) {
            mc_dbg_168_gm_clamped += 1U;
        }

        // Segmentation into blocks
        const uint32_t num_blocks = (max_delta + kAxisMoveMaxSteps - 1U) / kAxisMoveMaxSteps;

        g_move.max_delta       = max_delta;
        g_move.num_blocks      = num_blocks;
        g_move.current_block_k = 0U;
        g_move.start_hz        = start_hz;
        g_move.cruise_hz       = cruise_hz;
        g_move.accel_sps2      = accel_sps2;
        g_move.dir_mask        = dir_mask;

        for (uint8_t i = 0U; i < 4U; ++i) {
            g_move.total_delta[i]  = delta[i];
            g_move.start_pos[i]    = current_pos[i];
            g_move.target_pos[i]   = target_pos[i];
            g_move.spmm[i]         = spmm[i];
            g_move.queued_steps[i] = 0;
        }
    }

    // Phase 2: Queueing blocks
    while ((g_move.current_block_k < g_move.num_blocks) && (mc_plan_count() < (kPlanQueueSize - 1U))) {
        if (mc_is_killed() || (mc_dbg_80_mv_arm != 1U)) {
            for (uint8_t i = 0U; i < 4U; ++i) {
                const float added_mm = static_cast<float>(g_move.queued_steps[i]) / g_move.spmm[i];
                mc_position_set(i, g_move.start_pos[i] + added_mm);
            }
            record_refuse(6U);
            return GcodeStatus::Error;
        }

        const uint32_t k = g_move.current_block_k;
        const uint32_t n = g_move.num_blocks;
        MotionBlock b = {};
        b.start_hz   = g_move.start_hz;
        b.cruise_hz  = g_move.cruise_hz;
        b.accel_sps2 = g_move.accel_sps2;
        b.dir_mask   = g_move.dir_mask;

        bool block_has_steps = false;
        for (uint8_t i = 0U; i < 4U; ++i) {
            const uint64_t abs_d = static_cast<uint64_t>(labs(g_move.total_delta[i]));
            const uint32_t steps_k = static_cast<uint32_t>((abs_d * (k + 1U)) / n - (abs_d * k) / n);
            b.steps[i] = steps_k;
            if (steps_k != 0U) {
                block_has_steps = true;
            }
        }

        if (!block_has_steps) {
            g_move.current_block_k++;
            continue;
        }

        if (!mc_mq_push(b)) {
            for (uint8_t i = 0U; i < 4U; ++i) {
                const float added_mm = static_cast<float>(g_move.queued_steps[i]) / g_move.spmm[i];
                mc_position_set(i, g_move.start_pos[i] + added_mm);
            }
            record_refuse(6U);
            return GcodeStatus::Error;
        }

        g_move.current_block_k++;
        mc_dbg_167_gm_segments += 1U;
        mc_dbg_112_mq_run = 1U;

        for (uint8_t i = 0U; i < 4U; ++i) {
            if (b.steps[i] != 0U) {
                g_move.queued_steps[i] += (g_move.total_delta[i] >= 0) ? static_cast<int32_t>(b.steps[i])
                                                                       : -static_cast<int32_t>(b.steps[i]);
            }
        }
    }

    if (g_move.current_block_k < g_move.num_blocks) {
        return GcodeStatus::Busy;
    }

    for (uint8_t i = 0U; i < 4U; ++i) {
        mc_position_set(i, g_move.target_pos[i]);
    }
    mc_dbg_164_gm_result   = 1U;
    mc_dbg_165_gm_accepted += 1U;
    return GcodeStatus::Done;
}

}  // namespace mc
