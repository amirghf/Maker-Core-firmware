// بارگذاری و تخلیه فیلامنت M701 و M702 (بند ۹.۲). هیچ هدر ST اینجا include نمی‌شود.
#include "filament_load.h"
#include "filament_change.h"
#include "../config/mc_config.h"
#include "machine_state.h"
#include "motion_modes.h"
#include "motion_cmd.h"
#include "heater_ex.h"
#include "extruder.h"
#include "thermistor.h"
#include "filament_sensor.h"
#include "error.h"
#include "log.h"
#include "host_reply.h"
#include "hal_time.h"
#include "text_format.h"
#include <math.h>

extern "C" {
volatile uint32_t mc_dbg_451_fld_state      = 0;  // 0 Idle, 1 Heating, 2 Moving, 3 Purging
volatile uint32_t mc_dbg_452_fld_result     = 0;  // 0 هنوز نشده، 1 موفق، 2 قطع‌شده، 3 خطا
volatile uint32_t mc_dbg_453_fld_reason     = 0;  // 0 هیچ، 1 killed، 2 arm نیست، 3 هدف دما کم/صفر، 4 پارامتر نامعتبر، 5 حرکت رد شد، 6 گرم‌شدن ناموفق، 7 فعال بودن دستور دیگر، 8 هیتر در حالت تست، 9 فیلامنت تشخیص داده نشد
volatile uint32_t mc_dbg_454_fld_load_cnt   = 0;  // تعداد M701های شروع‌شده
volatile uint32_t mc_dbg_455_fld_unload_cnt = 0;  // تعداد M702های شروع‌شده

extern volatile uint32_t mc_dbg_80_mv_arm;
extern volatile uint32_t mc_dbg_445_fchg_ignore_sensor;
}

namespace mc {
namespace {

enum class FloadPhase : uint8_t {
    Idle    = 0,
    Heating = 1,
    Moving  = 2,
    Purging = 3
};

enum class SubMoveRes : uint8_t {
    Busy,
    Done,
    Error
};

FloadPhase g_phase = FloadPhase::Idle;

int32_t  g_saved_target_x10 = 0;
float    g_saved_e          = 0.0f;
float    g_saved_feed       = 1500.0f;

float    g_move_mm          = 0.0f;
float    g_purge_mm         = 0.0f;
bool     g_is_load          = true;

bool     g_sub_first        = true;
GcodeCommand g_sub_cmd       = {};

uint32_t g_heat_start_ms    = 0U;

GcodeStatus fload_refuse(uint32_t reason, const char* msg) {
    g_phase = FloadPhase::Idle;
    mc_dbg_451_fld_state  = 0U;
    mc_dbg_452_fld_result = 3U;
    mc_dbg_453_fld_reason = reason;
    mc_log_value(LogLevel::Error, msg, reason);
    return GcodeStatus::Error;
}

GcodeStatus fload_fail(uint32_t reason, const char* msg) {
    (void)mc_heater_ex_set_target_x10(0);
    mc_mode_feed_set(g_saved_feed);
    return fload_refuse(reason, msg);
}

GcodeStatus fload_success() {
    mc_position_set(kAxisE, g_saved_e);
    mc_mode_feed_set(g_saved_feed);
    g_phase = FloadPhase::Idle;
    mc_dbg_451_fld_state  = 0U;
    mc_dbg_452_fld_result = 1U;
    mc_dbg_453_fld_reason = 0U;
    return GcodeStatus::Done;
}

SubMoveRes exec_sub_move(const char* gcode_line) {
    if (g_sub_first) {
        if (mc_gcode_parse(gcode_line, g_sub_cmd) != GcodeResult::Ok) {
            return SubMoveRes::Error;
        }
    }
    const GcodeStatus status = mc_gmove_run(g_sub_cmd, g_sub_first);
    g_sub_first = false;

    if (status == GcodeStatus::Error) {
        return SubMoveRes::Error;
    }
    if (status == GcodeStatus::Busy) {
        return SubMoveRes::Busy;
    }
    if (mc_motion_busy()) {
        return SubMoveRes::Busy;
    }

    g_sub_first = true;
    return SubMoveRes::Done;
}

}  // namespace

void mc_fload_init() {
    g_phase     = FloadPhase::Idle;
    g_sub_first = true;

    mc_dbg_451_fld_state      = 0U;
    mc_dbg_452_fld_result     = 0U;
    mc_dbg_453_fld_reason     = 0U;
    mc_dbg_454_fld_load_cnt   = 0U;
    mc_dbg_455_fld_unload_cnt = 0U;
}

void mc_fload_poll() {
    // متغیرها نیاز به به‌روزرسانی دوره‌ای اضافی ندارند
}

bool mc_fload_active() {
    return g_phase != FloadPhase::Idle;
}

void mc_fload_abort() {
    g_phase     = FloadPhase::Idle;
    g_sub_first = true;
    mc_dbg_451_fld_state  = 0U;
    mc_dbg_452_fld_result = 2U;
}

GcodeStatus mc_fload_run(const GcodeCommand& cmd, bool first_call, bool is_load) {
    if (mc_is_killed()) {
        mc_fload_abort();
        return GcodeStatus::Error;
    }

    if (first_call) {
        // ۱) بررسی kill
        if (mc_is_killed()) {
            return fload_refuse(1U, is_load ? "M701 refused: killed" : "M702 refused: killed");
        }
        // ۲) بررسی arm
        if (mc_dbg_80_mv_arm != 1U) {
            return fload_refuse(2U, is_load ? "M701 refused: motion not armed" : "M702 refused: motion not armed");
        }
        // ۳) دستور دیگری فعال است
        if (mc_fload_active() || mc_filchg_active()) {
            return fload_refuse(7U, is_load ? "M701 refused: already active" : "M702 refused: already active");
        }
        // ۴) هیتر در حالت تست دیباگ
        if (mc_heater_ex_test_mode()) {
            return fload_refuse(8U, is_load ? "M701 refused: heater test mode" : "M702 refused: heater test mode");
        }

        // ۵) پارامترها
        if (mc_gcode_has(cmd, 'T')) {
            if (!mc_gcode_has_value(cmd, 'T') || (mc_gcode_int(cmd, 'T', 0) != 0)) {
                return fload_refuse(4U, is_load ? "M701 bad T param" : "M702 bad T param");
            }
        }

        float move_val = is_load ? MC_FILLOAD_LOAD_MM : MC_FILLOAD_UNLOAD_MM;
        const char char_param = is_load ? 'L' : 'U';

        if (mc_gcode_has(cmd, char_param)) {
            if (!mc_gcode_has_value(cmd, char_param)) {
                return fload_refuse(4U, is_load ? "M701 bad L param" : "M702 bad U param");
            }
            move_val = mc_gcode_value(cmd, char_param, 0.0f);
            if (isnan(move_val) || (move_val < 0.0f) || (move_val > MC_FILCHG_MAX_LEN_MM)) {
                return fload_refuse(4U, is_load ? "M701 bad L val" : "M702 bad U val");
            }
        }

        // ۶) هدف دما کافی باشد
        const int32_t target_x10 = mc_heater_ex_target_x10();
        if (target_x10 < (MC_EXTRUDE_MIN_TEMP_C * 10)) {
            mc_reply_line(is_load ? "echo:M701: set hotend temp first (M104/M109)" : "echo:M702: set hotend temp first (M104/M109)");
            return fload_refuse(3U, is_load ? "M701 target temp too low or zero" : "M702 target temp too low or zero");
        }

        // ۷) فقط برای M701: سنسور فیلامنت خالی نباشد
        if (is_load && mc_filament_empty() && (mc_dbg_445_fchg_ignore_sensor == 0U)) {
            mc_reply_line("echo:Filament not detected");
            return fload_refuse(9U, "M701 refused: filament not detected");
        }

        // ذخیره وضعیت اولیه
        g_saved_target_x10 = target_x10;
        g_saved_e          = mc_position_get(kAxisE);
        g_saved_feed       = mc_mode_feed_get_mmin();
        g_move_mm          = move_val;
        g_purge_mm         = is_load ? MC_FILLOAD_PURGE_MM : 0.0f;
        g_is_load          = is_load;
        g_sub_first        = true;

        if ((g_move_mm == 0.0f) && (g_purge_mm == 0.0f)) {
            return fload_success();
        }

        g_heat_start_ms = mc_millis();
        g_phase = FloadPhase::Heating;

        mc_dbg_451_fld_state  = 1U;
        mc_dbg_452_fld_result = 0U;
        mc_dbg_453_fld_reason = 0U;

        if (is_load) {
            mc_dbg_454_fld_load_cnt = mc_dbg_454_fld_load_cnt + 1U;
        } else {
            mc_dbg_455_fld_unload_cnt = mc_dbg_455_fld_unload_cnt + 1U;
        }

        return GcodeStatus::Busy;
    }

    switch (g_phase) {
        case FloadPhase::Heating: {
            int32_t cur_x10 = 0;
            const bool ok = mc_temp_get_x10(AdcChannel::TempExtruder, &cur_x10);
            if (!ok || mc_time_elapsed_ms(g_heat_start_ms, static_cast<uint32_t>(MC_FILCHG_HEAT_TIMEOUT_S) * 1000U)) {
                return fload_fail(6U, g_is_load ? "M701 heating failed or timeout" : "M702 heating failed or timeout");
            }
            if (cur_x10 >= (g_saved_target_x10 - (MC_FILCHG_HEAT_BAND_C * 10))) {
                if (g_move_mm > 0.0f) {
                    g_phase = FloadPhase::Moving;
                    mc_dbg_451_fld_state = 2U;
                    g_sub_first = true;
                } else if (g_purge_mm > 0.0f) {
                    g_phase = FloadPhase::Purging;
                    mc_dbg_451_fld_state = 3U;
                    g_sub_first = true;
                } else {
                    return fload_success();
                }
            }
            return GcodeStatus::Busy;
        }

        case FloadPhase::Moving: {
            const float delta_e  = g_is_load ? g_move_mm : -g_move_mm;
            const float e_target = mc_mode_e_relative_get() ? delta_e : (mc_position_get(kAxisE) + delta_e);
            const uint32_t feed  = g_is_load ? static_cast<uint32_t>(MC_FILCHG_LOAD_FEED_MM_MIN)
                                            : static_cast<uint32_t>(MC_FILCHG_RETRACT_FEED_MM_MIN);

            char line[64];
            uint16_t p = mc_fmt_text(line, sizeof(line), 0U, "G1 E");
            p = mc_fmt_float(line, sizeof(line), p, e_target, 3U);
            p = mc_fmt_text(line, sizeof(line), p, " F");
            p = mc_fmt_uint(line, sizeof(line), p, feed);

            const SubMoveRes res = exec_sub_move(line);
            if (res == SubMoveRes::Error) {
                return fload_fail(5U, g_is_load ? "M701 move failed" : "M702 move failed");
            }
            if (res == SubMoveRes::Busy) {
                return GcodeStatus::Busy;
            }

            if (g_is_load && (g_purge_mm > 0.0f)) {
                g_phase = FloadPhase::Purging;
                mc_dbg_451_fld_state = 3U;
                g_sub_first = true;
                return GcodeStatus::Busy;
            }

            return fload_success();
        }

        case FloadPhase::Purging: {
            const float purge_mm = g_purge_mm;
            const float e_target = mc_mode_e_relative_get() ? purge_mm : (mc_position_get(kAxisE) + purge_mm);
            char line[64];
            uint16_t p = mc_fmt_text(line, sizeof(line), 0U, "G1 E");
            p = mc_fmt_float(line, sizeof(line), p, e_target, 3U);
            p = mc_fmt_text(line, sizeof(line), p, " F");
            p = mc_fmt_uint(line, sizeof(line), p, static_cast<uint32_t>(MC_FILCHG_PURGE_FEED_MM_MIN));

            const SubMoveRes res = exec_sub_move(line);
            if (res == SubMoveRes::Error) {
                return fload_fail(5U, "M701 purge move failed");
            }
            if (res == SubMoveRes::Busy) {
                return GcodeStatus::Busy;
            }

            return fload_success();
        }

        case FloadPhase::Idle:
        default:
            return fload_refuse(7U, "M701/M702 unknown phase");
    }
}

}  // namespace mc
