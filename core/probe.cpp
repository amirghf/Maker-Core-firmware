// کاوش با پراب BLTouch و آفست پراب (بند ۶.۶). جزئیات در probe.h.
#include "probe.h"
#include "../config/mc_config.h"
#include "bltouch.h"
#include "machine_state.h"
#include "motion_modes.h"
#include "motion_limits.h"
#include "motion_queue.h"
#include "planner.h"
#include "axis_move.h"
#include "stepper_out.h"
#include "hal_step_timer.h"
#include "endstop.h"
#include "endstop_guard.h"
#include "step_gen.h"
#include "error.h"
#include "log.h"
#include "host_reply.h"
#include "text_format.h"
#include <math.h>
#include <stdlib.h>

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند. شماره‌ها دائمی‌اند.
extern "C" {
volatile uint32_t mc_dbg_236_pr_state        = 0;  // خواندنی: وضعیت G30 (0 بیکار، 1 در حال کار، 2 تمام شد، 3 خطا)
volatile uint32_t mc_dbg_237_pr_stage        = 0;  // خواندنی: مرحله (0 هیچ، 1 WaitIdle، 2 Deploy، 3 Approach، 4 Return، 5 Stow، 6 AlarmRelease، 7 StowAgain، 8 PreRelease، 9 PreStow)
volatile uint32_t mc_dbg_238_pr_result       = 0;  // خواندنی: نتیجه آخرین G30 (0 هیچ، 1 موفق، 2 kill یا arm نیست، 3 فعال‌شدن درایور Z ناموفق، 4 سیگنال قبل از Deploy بالا بود، 5 سیگنال بعد از Deploy بالا بود، 6 پراب نخورد، 7 صف رد کرد، 8 BLTouch فرمان را نپذیرفت یا Deployed نشد، 9 توقف غیرمنتظره، 10 سیگنال بعد از Stow بالا بود، 11 پارامتر نامعتبر)
volatile uint32_t mc_dbg_239_pr_runs         = 0;  // خواندنی: تعداد G30 هایی که موفق تمام شدند
volatile uint32_t mc_dbg_240_pr_cap_mm       = 0;  // نوشتنی: سقف مسافت پایین‌رفتن (میلی‌متر). 0 = پیش‌فرض (kProbeMaxMm)
volatile int32_t  mc_dbg_241_pr_start_um     = 0;  // خواندنی: موقعیت Z در شروع آخرین G30 (میکرومتر)
volatile int32_t  mc_dbg_242_pr_trigger_um   = 0;  // خواندنی: موقعیت Z در لحظه برخورد پراب در آخرین G30 (میکرومتر)
volatile uint32_t mc_dbg_243_pr_travel_um    = 0;  // خواندنی: مسافت پایین‌رفتن تا برخورد (میکرومتر)
volatile int32_t  mc_dbg_244_pr_off_x_um     = 0;  // خواندنی: آفست X (میکرومتر)
volatile int32_t  mc_dbg_245_pr_off_y_um     = 0;  // خواندنی: آفست Y (میکرومتر)
volatile int32_t  mc_dbg_246_pr_off_z_um     = 0;  // خواندنی: آفست Z (میکرومتر)
volatile uint32_t mc_dbg_247_pr_post_stow_hi = 0;  // خواندنی: 1 یعنی در آخرین G30 بعد از اولین Stow سیگنال پراب هنوز بالا بود
volatile uint32_t mc_dbg_248_pr_refused      = 0;  // خواندنی: شمارنده دستورهای رد شده
volatile uint32_t mc_dbg_448_pr_prerel_cnt   = 0;  // خواندنی: تعداد دفعاتی که قبل از G30، توالی AlarmRelease و Stow شروع شد (از روشن شدن) (بند ۶.۶)

extern volatile uint32_t mc_dbg_80_mv_arm;
extern volatile uint32_t mc_dbg_112_mq_run;
}

namespace mc {
namespace {

enum class Stage : uint8_t {
    Idle         = 0,
    WaitIdle     = 1,
    Deploy       = 2,
    Approach     = 3,
    Return       = 4,
    Stow         = 5,
    AlarmRelease = 6,
    StowAgain    = 7,
    PreRelease   = 8,   // ریست آلارم قبل از Deploy در G30 (بند ۶.۷ و ۶.۶)
    PreStow      = 9    // جمع کردن پین بعد از PreRelease و قبل از Deploy در G30 (بند ۶.۷ و ۶.۶)
};

constexpr uint32_t kStateIdle  = 0U;
constexpr uint32_t kStateBusy  = 1U;
constexpr uint32_t kStateDone  = 2U;
constexpr uint32_t kStateError = 3U;

constexpr uint32_t kResOk                    = 1U;
constexpr uint32_t kResNotArmed              = 2U;
constexpr uint32_t kResDriverFail           = 3U;
constexpr uint32_t kResSignalPreDeployHigh  = 4U;
constexpr uint32_t kResSignalPostDeployHigh = 5U;
constexpr uint32_t kResNoHitCap              = 6U;
constexpr uint32_t kResQueueRefused         = 7U;
constexpr uint32_t kResBltouchFail          = 8U;
constexpr uint32_t kResUnexpectedStop       = 9U;
constexpr uint32_t kResSignalPostStowHigh   = 10U;
constexpr uint32_t kResBadParam             = 11U;

struct Ctx {
    Stage    stage;
    bool     forward;
    uint32_t total_steps;
    uint32_t queued_steps;
    uint32_t hit_seq;
    float    feed_mm_s;
    int32_t  start_steps;
    float    start_z;
    uint32_t travel_steps;
    float    trigger_z;
    uint32_t err;
};

Ctx   g           = {};
float g_offset[3] = { MC_PROBE_OFFSET_X_MM, MC_PROBE_OFFSET_Y_MM, MC_PROBE_OFFSET_Z_MM };

uint32_t clamp_u32(long v, uint32_t lo, uint32_t hi) {
    if (v < static_cast<long>(lo)) { return lo; }
    if (v > static_cast<long>(hi)) { return hi; }
    return static_cast<uint32_t>(v);
}

uint32_t mm_to_steps_z(float mm) {
    return clamp_u32(lroundf(mm * mc_mode_spmm_get(kAxisZ)), 1U, 100000000U);
}

void refresh_debug() {
    mc_dbg_236_pr_state    = (g.stage == Stage::Idle) ? ((mc_dbg_238_pr_result == kResOk) ? kStateDone : ((mc_dbg_238_pr_result != 0U) ? kStateError : kStateIdle)) : kStateBusy;
    mc_dbg_237_pr_stage    = static_cast<uint32_t>(g.stage);
    mc_dbg_244_pr_off_x_um = static_cast<int32_t>(lroundf(g_offset[0] * 1000.0f));
    mc_dbg_245_pr_off_y_um = static_cast<int32_t>(lroundf(g_offset[1] * 1000.0f));
    mc_dbg_246_pr_off_z_um = static_cast<int32_t>(lroundf(g_offset[2] * 1000.0f));
}

bool push_blocks_z() {
    const float spmm = mc_mode_spmm_get(kAxisZ);
    float feed = g.feed_mm_s;
    const float vmax = mc_mlim_feed_get(kAxisZ);
    if (feed > vmax) { feed = vmax; }

    const uint32_t cruise = clamp_u32(lroundf(feed * spmm), 1U, kAxisMoveMaxRateHz);
    uint32_t start = clamp_u32(lroundf(mc_mlim_minfeed_get() * spmm), kStepTimerMinHz, cruise);
    if (start > cruise) { start = cruise; }
    const uint32_t accel = clamp_u32(lroundf(mc_mlim_acc_get(kAxisZ) * spmm), kAxisAccelMinSps2, kAxisAccelMaxSps2);

    while ((g.queued_steps < g.total_steps) && (mc_plan_count() < (kPlanQueueSize - 1U))) {
        uint32_t n = g.total_steps - g.queued_steps;
        if (n > kAxisMoveMaxSteps) { n = kAxisMoveMaxSteps; }

        MotionBlock b = {};
        b.steps[kAxisZ] = n;
        b.start_hz      = start;
        b.cruise_hz     = cruise;
        b.accel_sps2    = accel;
        b.dir_mask      = g.forward ? static_cast<uint8_t>(1U << kAxisZ) : 0U;

        if (!mc_mq_push(b)) { return false; }
        mc_dbg_112_mq_run = 1U;
        g.queued_steps += n;
    }
    return true;
}

void start_move_z(Stage s, bool forward, uint32_t steps, float feed_mm_s) {
    g.stage        = s;
    g.forward      = forward;
    g.total_steps  = steps;
    g.queued_steps = 0U;
    g.feed_mm_s    = feed_mm_s;
    g.hit_seq      = mc_stepgen_endstop_hits();
    mc_dbg_237_pr_stage = static_cast<uint32_t>(s);
}

GcodeStatus fail_g30(uint32_t code, const char* text, bool try_stow = true) {
    if ((g.stage == Stage::Approach) || (g.stage == Stage::Return)) {
        mc_mq_clear();
        mc_eguard_resync();
    }
    if (try_stow) {
        (void)mc_bltouch_start(BltouchCmd::Stow);
    }
    g.stage = Stage::Idle;
    mc_dbg_236_pr_state  = kStateError;
    mc_dbg_237_pr_stage  = 0U;
    mc_dbg_238_pr_result = code;
    refresh_debug();
    mc_log_value(LogLevel::Error, text, code);
    return GcodeStatus::Error;
}

GcodeStatus finish_g30() {
    g.stage = Stage::Idle;
    mc_dbg_237_pr_stage = 0U;
    if (g.err != 0U) {
        mc_dbg_236_pr_state  = kStateError;
        mc_dbg_238_pr_result = g.err;
        refresh_debug();
        mc_log_value(LogLevel::Error, "G30 error", g.err);
        return GcodeStatus::Error;
    }

    const float z_bed = g.trigger_z + g_offset[2];
    char buf[128];
    uint16_t n = 0U;
    const uint16_t cap = static_cast<uint16_t>(sizeof(buf));
    n = mc_fmt_text(buf, cap, n, "Bed X:");
    n = mc_fmt_float(buf, cap, n, mc_position_get(kAxisX), 2U);
    n = mc_fmt_text(buf, cap, n, " Y:");
    n = mc_fmt_float(buf, cap, n, mc_position_get(kAxisY), 2U);
    n = mc_fmt_text(buf, cap, n, " Z:");
    n = mc_fmt_float(buf, cap, n, z_bed, 3U);
    mc_reply_line(buf);

    mc_dbg_239_pr_runs   = mc_dbg_239_pr_runs + 1U;
    mc_dbg_238_pr_result = kResOk;
    mc_dbg_236_pr_state  = kStateDone;
    refresh_debug();
    return GcodeStatus::Done;
}

}  // namespace

void mc_probe_init() {
    g = Ctx{};
    g.stage = Stage::Idle;
    g_offset[0] = MC_PROBE_OFFSET_X_MM;
    g_offset[1] = MC_PROBE_OFFSET_Y_MM;
    g_offset[2] = MC_PROBE_OFFSET_Z_MM;
    mc_dbg_236_pr_state        = kStateIdle;
    mc_dbg_237_pr_stage        = 0U;
    mc_dbg_238_pr_result       = 0U;
    mc_dbg_239_pr_runs         = 0U;
    mc_dbg_240_pr_cap_mm       = 0U;
    mc_dbg_241_pr_start_um     = 0;
    mc_dbg_242_pr_trigger_um   = 0;
    mc_dbg_243_pr_travel_um    = 0U;
    mc_dbg_244_pr_off_x_um     = 0;
    mc_dbg_245_pr_off_y_um     = 0;
    mc_dbg_246_pr_off_z_um     = 0;
    mc_dbg_247_pr_post_stow_hi = 0U;
    mc_dbg_248_pr_refused      = 0U;
    mc_dbg_448_pr_prerel_cnt   = 0U;
    refresh_debug();
}

void mc_probe_poll() {
    refresh_debug();
}

GcodeStatus mc_probe_run(const GcodeCommand& cmd, bool first_call) {
    if (first_call) {
        g = Ctx{};
        g.stage = Stage::WaitIdle;
        mc_dbg_236_pr_state        = kStateBusy;
        mc_dbg_237_pr_stage        = 1U;
        mc_dbg_238_pr_result       = 0U;
        mc_dbg_241_pr_start_um     = 0;
        mc_dbg_242_pr_trigger_um   = 0;
        mc_dbg_243_pr_travel_um    = 0U;
        mc_dbg_247_pr_post_stow_hi = 0U;

        if (mc_gcode_has(cmd, 'X') || mc_gcode_has(cmd, 'Y') || mc_gcode_has(cmd, 'Z') ||
            mc_gcode_has(cmd, 'E') || mc_gcode_has(cmd, 'S') || mc_gcode_has(cmd, 'P') ||
            mc_gcode_has(cmd, 'I') || mc_gcode_has(cmd, 'J')) {
            mc_probe_note_refused();
            return fail_g30(kResBadParam, "G30: extra parameter refused", false);
        }
    }

    switch (g.stage) {
        case Stage::WaitIdle:
            if (mc_mq_busy()) { return GcodeStatus::Busy; }
            if (mc_is_killed() || (mc_dbg_80_mv_arm != 1U)) {
                return fail_g30(kResNotArmed, "G30: kill or arm off", false);
            }
            mc_eguard_snapshot();
            g.start_z     = mc_position_get(kAxisZ);
            g.start_steps = mc_axis_position_steps(StepAxis::Z);
            mc_dbg_241_pr_start_um = static_cast<int32_t>(lroundf(g.start_z * 1000.0f));

            if (!mc_motor_enable(StepAxis::Z, true)) {
                return fail_g30(kResDriverFail, "G30: driver enable failed", false);
            }

            g.stage = Stage::PreRelease;
            mc_dbg_237_pr_stage = 8U;
            mc_dbg_448_pr_prerel_cnt = mc_dbg_448_pr_prerel_cnt + 1U;
            if (!mc_bltouch_start(BltouchCmd::AlarmRelease)) {
                return fail_g30(kResBltouchFail, "G30: bltouch pre-release start failed", false);
            }
            return GcodeStatus::Busy;

        case Stage::PreRelease:
            if (mc_bltouch_busy()) { return GcodeStatus::Busy; }
            g.stage = Stage::PreStow;
            mc_dbg_237_pr_stage = 9U;
            if (!mc_bltouch_start(BltouchCmd::Stow)) {
                return fail_g30(kResBltouchFail, "G30: bltouch pre-stow start failed", true);
            }
            return GcodeStatus::Busy;

        case Stage::PreStow:
            if (mc_bltouch_busy()) { return GcodeStatus::Busy; }
            if (mc_endstop_triggered(2U)) {
                return fail_g30(kResSignalPreDeployHigh, "G30: probe signal high before deploy", false);
            }
            g.stage = Stage::Deploy;
            mc_dbg_237_pr_stage = 2U;
            if (!mc_bltouch_start(BltouchCmd::Deploy)) {
                return fail_g30(kResBltouchFail, "G30: bltouch deploy start failed", false);
            }
            return GcodeStatus::Busy;

        case Stage::Deploy:
            if (mc_bltouch_busy()) { return GcodeStatus::Busy; }
            if (mc_bltouch_pin_state() != BltouchPin::Deployed) {
                return fail_g30(kResBltouchFail, "G30: bltouch pin state not deployed", false);
            }
            if (mc_endstop_triggered(2U)) {
                g.err = kResSignalPostDeployHigh;
                g.stage = Stage::Stow;
                mc_dbg_237_pr_stage = 5U;
                if (!mc_bltouch_start(BltouchCmd::Stow)) {
                    return fail_g30(kResBltouchFail, "G30: bltouch stow start failed", false);
                }
                return GcodeStatus::Busy;
            }
            {
                float cap_mm = kProbeMaxMm;
                const uint32_t ov = mc_dbg_240_pr_cap_mm;
                if ((ov > 0U) && (static_cast<float>(ov) < cap_mm)) {
                    cap_mm = static_cast<float>(ov);
                }
                start_move_z(Stage::Approach, false, mm_to_steps_z(cap_mm), kProbeApproachMmS);
            }
            return GcodeStatus::Busy;

        case Stage::Approach: {
            if (mc_is_killed() || (mc_dbg_80_mv_arm != 1U)) {
                return fail_g30(kResNotArmed, "G30: kill or arm off");
            }

            const bool busy_before = mc_mq_busy();
            mc_eguard_poll();
            const bool hit = (mc_stepgen_endstop_hits() != g.hit_seq);

            if (hit) {
                if (mc_mq_busy()) { return GcodeStatus::Busy; }
                if ((mc_stepgen_endstop_last_mask() & (1U << 2U)) == 0U) {
                    return fail_g30(kResUnexpectedStop, "G30: wrong endstop hit");
                }
                const int32_t curr_steps = mc_axis_position_steps(StepAxis::Z);
                g.travel_steps = static_cast<uint32_t>(labs(g.start_steps - curr_steps));
                const float spmm = mc_mode_spmm_get(kAxisZ);
                const float travel_mm = static_cast<float>(g.travel_steps) / spmm;
                g.trigger_z = g.start_z - travel_mm;

                mc_dbg_242_pr_trigger_um = static_cast<int32_t>(lroundf(g.trigger_z * 1000.0f));
                mc_dbg_243_pr_travel_um  = static_cast<uint32_t>(lroundf(travel_mm * 1000.0f));

                if (g.travel_steps > 0U) {
                    start_move_z(Stage::Return, true, g.travel_steps, kProbeReturnMmS);
                } else {
                    g.stage = Stage::Stow;
                    mc_dbg_237_pr_stage = 5U;
                    if (!mc_bltouch_start(BltouchCmd::Stow)) {
                        return fail_g30(kResBltouchFail, "G30: bltouch stow start failed", false);
                    }
                }
                return GcodeStatus::Busy;
            }

            if (g.queued_steps < g.total_steps) {
                if (!push_blocks_z()) {
                    return fail_g30(kResQueueRefused, "G30: queue refused");
                }
                return GcodeStatus::Busy;
            }

            if (busy_before) { return GcodeStatus::Busy; }

            g.err = kResNoHitCap;
            const int32_t curr_steps = mc_axis_position_steps(StepAxis::Z);
            g.travel_steps = static_cast<uint32_t>(labs(g.start_steps - curr_steps));
            if (g.travel_steps > 0U) {
                start_move_z(Stage::Return, true, g.travel_steps, kProbeReturnMmS);
            } else {
                g.stage = Stage::Stow;
                mc_dbg_237_pr_stage = 5U;
                if (!mc_bltouch_start(BltouchCmd::Stow)) {
                    return fail_g30(kResBltouchFail, "G30: bltouch stow start failed", false);
                }
            }
            return GcodeStatus::Busy;
        }

        case Stage::Return: {
            if (mc_is_killed() || (mc_dbg_80_mv_arm != 1U)) {
                return fail_g30(kResNotArmed, "G30: kill or arm off");
            }

            const bool busy_before = mc_mq_busy();
            mc_eguard_poll();
            if (mc_stepgen_endstop_hits() != g.hit_seq) {
                return fail_g30(kResUnexpectedStop, "G30: unexpected stop while returning");
            }

            if (g.queued_steps < g.total_steps) {
                if (!push_blocks_z()) {
                    return fail_g30(kResQueueRefused, "G30: queue refused");
                }
                return GcodeStatus::Busy;
            }

            if (busy_before) { return GcodeStatus::Busy; }

            mc_position_set(kAxisZ, g.start_z);
            mc_eguard_snapshot();

            g.stage = Stage::Stow;
            mc_dbg_237_pr_stage = 5U;
            if (!mc_bltouch_start(BltouchCmd::Stow)) {
                return fail_g30(kResBltouchFail, "G30: bltouch stow start failed", false);
            }
            return GcodeStatus::Busy;
        }

        case Stage::Stow:
            if (mc_bltouch_busy()) { return GcodeStatus::Busy; }
            if (mc_endstop_triggered(2U)) {
                mc_dbg_247_pr_post_stow_hi = 1U;
                g.stage = Stage::AlarmRelease;
                mc_dbg_237_pr_stage = 6U;
                if (!mc_bltouch_start(BltouchCmd::AlarmRelease)) {
                    return fail_g30(kResBltouchFail, "G30: bltouch alarm release start failed", false);
                }
                return GcodeStatus::Busy;
            }
            return finish_g30();

        case Stage::AlarmRelease:
            if (mc_bltouch_busy()) { return GcodeStatus::Busy; }
            g.stage = Stage::StowAgain;
            mc_dbg_237_pr_stage = 7U;
            if (!mc_bltouch_start(BltouchCmd::Stow)) {
                return fail_g30(kResBltouchFail, "G30: bltouch stow again start failed", false);
            }
            return GcodeStatus::Busy;

        case Stage::StowAgain:
            if (mc_bltouch_busy()) { return GcodeStatus::Busy; }
            if (mc_endstop_triggered(2U)) {
                if (g.err == 0U) {
                    g.err = kResSignalPostStowHigh;
                }
            }
            return finish_g30();

        case Stage::Idle:
        default:
            return fail_g30(kResUnexpectedStop, "G30: bad stage", false);
    }
}

float mc_probe_offset_get(uint8_t axis) {
    if (axis > 2U) { return 0.0f; }
    return g_offset[axis];
}

bool mc_probe_offset_valid(uint8_t axis, float mm) {
    if (axis > 2U) { return false; }
    if (isnan(mm)) { return false; }
    const float limit = (axis == 2U) ? kProbeOffsetZMaxMm : kProbeOffsetXYMaxMm;
    return (mm >= -limit) && (mm <= limit);
}

bool mc_probe_offset_set(uint8_t axis, float mm) {
    if (!mc_probe_offset_valid(axis, mm)) { return false; }
    g_offset[axis] = mm;
    refresh_debug();
    return true;
}

void mc_probe_note_refused() {
    mc_dbg_248_pr_refused = mc_dbg_248_pr_refused + 1U;
    refresh_debug();
}

}  // namespace mc
