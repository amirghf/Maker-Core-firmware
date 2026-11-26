// تعویض فیلامنت M600 (بند ۹.۱). هیچ هدر ST اینجا include نمی‌شود.
#include "filament_change.h"
#include "../config/mc_config.h"
#include "machine_state.h"
#include "motion_modes.h"
#include "motion_cmd.h"
#include "heater_ex.h"
#include "extruder.h"
#include "thermistor.h"
#include "filament_sensor.h"
#include "homing.h"
#include "soft_limits.h"
#include "error.h"
#include "log.h"
#include "host_reply.h"
#include "hal_time.h"
#include "hal_gpio.h"
#include "hal_buzzer.h"
#include "text_format.h"
#include "alert.h"
#include <math.h>

extern "C" {
volatile uint32_t mc_dbg_434_fchg_state          = 0;  // 0 Idle, 1 Heating, 2 Retract, 3 Park, 4 Unload, 5 WaitUser, 6 Reheat, 7 Load, 8 Purge, 9 Return
volatile uint32_t mc_dbg_435_fchg_result         = 0;  // 0 هنوز نشده، 1 موفق، 2 قطع‌شده، 3 خطا
volatile uint32_t mc_dbg_436_fchg_reason         = 0;  // 0 هیچ، 1 killed، 2 arm نیست، 3 هدف دما کم/صفر، 4 پارامتر نامعتبر، 5 حرکت رد شد، 6 گرم‌شدن ناموفق، 7 M600 فعال است، 8 هیتر در حالت تست
volatile uint32_t mc_dbg_437_fchg_resume_src     = 0;  // 0 هیچ، 1 دکمه نمایشگر، 2 کلید START، 3 M108
volatile uint32_t mc_dbg_438_fchg_resume_cnt     = 0;  // تعداد «ادامه»های پذیرفته‌شده
volatile uint32_t mc_dbg_439_fchg_resume_refused = 0;  // تعداد «ادامه»های ردشده به‌خاطر سنسور فیلامنت
volatile uint32_t mc_dbg_440_fchg_run_cnt        = 0;  // تعداد M600های شروع‌شده
volatile uint32_t mc_dbg_441_fchg_cooled         = 0;  // 1 = هیتر در انتظار خاموش شده
volatile uint32_t mc_dbg_442_fchg_wait_s         = 0;  // ثانیه‌های سپری‌شده در انتظار
volatile uint32_t mc_dbg_443_fchg_beep_cnt       = 0;  // تعداد بوق‌های یادآوری
volatile uint32_t mc_dbg_444_fchg_start_key      = 0;  // وضعیت کلید START (1 = فشرده، 0 = رها)
volatile uint32_t mc_dbg_445_fchg_ignore_sensor  = 0;  // 1 = نادیده‌گرفتن سنسور فیلامنت (پیش‌فرض 0)
volatile uint32_t mc_dbg_446_fchg_tmo_s          = 0;  // مدت نگه‌داشتن هیتر در انتظار (0 = از کانفیگ)

extern volatile uint32_t mc_dbg_80_mv_arm;
}

namespace mc {
namespace {

enum class FilChgPhase : uint8_t {
    Idle     = 0,
    Heating  = 1,
    Retract  = 2,
    Park     = 3,
    Unload   = 4,
    WaitUser = 5,
    Reheat   = 6,
    Load     = 7,
    Purge    = 8,
    Return   = 9
};

enum class SubMoveRes : uint8_t {
    Busy,
    Done,
    Error
};

FilChgPhase g_phase = FilChgPhase::Idle;

int32_t  g_saved_target_x10 = 0;
float    g_saved_x          = 0.0f;
float    g_saved_y          = 0.0f;
float    g_saved_z          = 0.0f;
float    g_saved_e          = 0.0f;
float    g_saved_feed       = 1500.0f;

float    g_retract_mm = MC_FILCHG_RETRACT_MM;
float    g_unload_mm  = MC_FILCHG_UNLOAD_MM;
float    g_load_mm    = MC_FILCHG_LOAD_MM;
float    g_park_x     = MC_FILCHG_PARK_X_MM;
float    g_park_y     = MC_FILCHG_PARK_Y_MM;
float    g_z_lift     = MC_FILCHG_Z_LIFT_MM;

bool     g_parked_z   = false;
bool     g_parked_xy  = false;

uint8_t  g_sub_step   = 0;
bool     g_sub_first  = true;

GcodeCommand g_sub_cmd = {};

uint32_t g_heat_start_ms     = 0U;
uint32_t g_wait_start_ms     = 0U;
uint32_t g_last_beep_ms      = 0U;
uint32_t g_debounce_start_ms = 0U;

bool     g_last_key_raw      = false;
bool     g_debounced_key     = false;
bool     g_key_initial_hold  = false;

uint8_t  g_resume_src        = 0U;
bool     g_resume_requested  = false;

GcodeStatus filchg_fail(uint32_t reason, const char* msg) {
    (void)mc_heater_ex_set_target_x10(0);
    mc_mode_feed_set(g_saved_feed);
    g_phase = FilChgPhase::Idle;
    mc_dbg_434_fchg_state  = 0U;
    mc_dbg_435_fchg_result = 3U;
    mc_dbg_436_fchg_reason = reason;
    mc_log_value(LogLevel::Error, msg, reason);
    mc_alert_raise_id(AlertId::FilChgFailed, msg, reason);
    return GcodeStatus::Error;
}

GcodeStatus filchg_success() {
    mc_position_set(kAxisE, g_saved_e);
    mc_mode_feed_set(g_saved_feed);
    g_phase = FilChgPhase::Idle;
    mc_dbg_434_fchg_state  = 0U;
    mc_dbg_435_fchg_result = 1U;
    mc_dbg_436_fchg_reason = 0U;
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

void mc_filchg_init() {
    g_phase = FilChgPhase::Idle;
    g_sub_step  = 0U;
    g_sub_first = true;
    g_resume_requested = false;
    g_resume_src       = 0U;

    mc_dbg_434_fchg_state          = 0U;
    mc_dbg_435_fchg_result         = 0U;
    mc_dbg_436_fchg_reason         = 0U;
    mc_dbg_437_fchg_resume_src     = 0U;
    mc_dbg_438_fchg_resume_cnt     = 0U;
    mc_dbg_439_fchg_resume_refused = 0U;
    mc_dbg_440_fchg_run_cnt        = 0U;
    mc_dbg_441_fchg_cooled         = 0U;
    mc_dbg_442_fchg_wait_s         = 0U;
    mc_dbg_443_fchg_beep_cnt       = 0U;
    mc_dbg_444_fchg_start_key      = 0U;
    mc_dbg_445_fchg_ignore_sensor  = 0U;
    mc_dbg_446_fchg_tmo_s          = 0U;
}

void mc_filchg_poll() {
    (void)mc_dbg_445_fchg_ignore_sensor;
    (void)mc_dbg_446_fchg_tmo_s;

    if (g_phase == FilChgPhase::WaitUser) {
        mc_dbg_442_fchg_wait_s = (mc_millis() - g_wait_start_ms) / 1000U;
    }
}

bool mc_filchg_active() {
    return g_phase != FilChgPhase::Idle;
}

uint8_t mc_filchg_phase() {
    return static_cast<uint8_t>(g_phase);
}

bool mc_filchg_waiting_user() {
    return g_phase == FilChgPhase::WaitUser;
}

bool mc_filchg_can_resume() {
    return g_phase == FilChgPhase::WaitUser;
}

void mc_filchg_resume_request(uint8_t src) {
    if (mc_filchg_can_resume()) {
        g_resume_src       = src;
        g_resume_requested = true;
    }
}

void mc_filchg_abort() {
    g_phase = FilChgPhase::Idle;
    g_sub_step  = 0U;
    g_sub_first = true;
    g_resume_requested = false;
    mc_dbg_434_fchg_state  = 0U;
    mc_dbg_435_fchg_result = 2U;
}

GcodeStatus mc_filchg_run(const GcodeCommand& cmd, bool first_call) {
    if (mc_is_killed()) {
        mc_filchg_abort();
        return GcodeStatus::Error;
    }

    if (first_call) {
        // ۱) بررسی kill
        if (mc_is_killed()) {
            return filchg_fail(1U, "M600 refused: killed");
        }
        // ۲) بررسی arm
        if (mc_dbg_80_mv_arm != 1U) {
            return filchg_fail(2U, "M600 refused: motion not armed");
        }
        // ۳) M600 دیگری فعال است
        if (mc_filchg_active()) {
            return filchg_fail(7U, "M600 refused: already active");
        }
        // ۴) هیتر در حالت تست دیباگ
        if (mc_heater_ex_test_mode()) {
            return filchg_fail(8U, "M600 refused: heater test mode");
        }

        // ۵) پارامترها
        float retract_val = MC_FILCHG_RETRACT_MM;
        float unload_val  = MC_FILCHG_UNLOAD_MM;
        float load_val    = MC_FILCHG_LOAD_MM;
        float park_x_val  = MC_FILCHG_PARK_X_MM;
        float park_y_val  = MC_FILCHG_PARK_Y_MM;
        float z_lift_val  = MC_FILCHG_Z_LIFT_MM;

        if (mc_gcode_has(cmd, 'T')) {
            if (!mc_gcode_has_value(cmd, 'T') || (mc_gcode_int(cmd, 'T', 0) != 0)) {
                return filchg_fail(4U, "M600 bad T param");
            }
        }

        if (mc_gcode_has(cmd, 'E')) {
            if (!mc_gcode_has_value(cmd, 'E')) { return filchg_fail(4U, "M600 bad E"); }
            retract_val = mc_gcode_value(cmd, 'E', 0.0f);
            if (isnan(retract_val) || (retract_val < 0.0f) || (retract_val > MC_FILCHG_MAX_LEN_MM)) {
                return filchg_fail(4U, "M600 bad E val");
            }
        }

        if (mc_gcode_has(cmd, 'U')) {
            if (!mc_gcode_has_value(cmd, 'U')) { return filchg_fail(4U, "M600 bad U"); }
            unload_val = mc_gcode_value(cmd, 'U', 0.0f);
            if (isnan(unload_val) || (unload_val < 0.0f) || (unload_val > MC_FILCHG_MAX_LEN_MM)) {
                return filchg_fail(4U, "M600 bad U val");
            }
        }

        if (mc_gcode_has(cmd, 'L')) {
            if (!mc_gcode_has_value(cmd, 'L')) { return filchg_fail(4U, "M600 bad L"); }
            load_val = mc_gcode_value(cmd, 'L', 0.0f);
            if (isnan(load_val) || (load_val < 0.0f) || (load_val > MC_FILCHG_MAX_LEN_MM)) {
                return filchg_fail(4U, "M600 bad L val");
            }
        }

        if (mc_gcode_has(cmd, 'Z')) {
            if (!mc_gcode_has_value(cmd, 'Z')) { return filchg_fail(4U, "M600 bad Z"); }
            z_lift_val = mc_gcode_value(cmd, 'Z', 0.0f);
            if (isnan(z_lift_val) || (z_lift_val < 0.0f) || (z_lift_val > MC_FILCHG_MAX_LEN_MM)) {
                return filchg_fail(4U, "M600 bad Z val");
            }
        }

        if (mc_gcode_has(cmd, 'X')) {
            if (!mc_gcode_has_value(cmd, 'X')) { return filchg_fail(4U, "M600 bad X"); }
            park_x_val = mc_gcode_value(cmd, 'X', 0.0f);
            if (isnan(park_x_val) || (fabsf(park_x_val) > kModeMaxPositionMm)) {
                return filchg_fail(4U, "M600 bad X val");
            }
        }

        if (mc_gcode_has(cmd, 'Y')) {
            if (!mc_gcode_has_value(cmd, 'Y')) { return filchg_fail(4U, "M600 bad Y"); }
            park_y_val = mc_gcode_value(cmd, 'Y', 0.0f);
            if (isnan(park_y_val) || (fabsf(park_y_val) > kModeMaxPositionMm)) {
                return filchg_fail(4U, "M600 bad Y val");
            }
        }

        // ۶) هدف دما کافی باشد
        const int32_t target_x10 = mc_heater_ex_target_x10();
        if (target_x10 < (MC_EXTRUDE_MIN_TEMP_C * 10)) {
            mc_reply_line("echo:M600: set hotend temp first (M104/M109)");
            mc_alert_raise_id(AlertId::HotendTempNeeded);
            return filchg_fail(3U, "M600 target temp too low or zero");
        }

        // ذخیره وضعیت
        g_saved_target_x10 = target_x10;
        g_saved_x    = mc_position_get(kAxisX);
        g_saved_y    = mc_position_get(kAxisY);
        g_saved_z    = mc_position_get(kAxisZ);
        g_saved_e    = mc_position_get(kAxisE);
        g_saved_feed = mc_mode_feed_get_mmin();

        g_retract_mm = retract_val;
        g_unload_mm  = unload_val;
        g_load_mm    = load_val;
        g_park_x     = park_x_val;
        g_park_y     = park_y_val;
        g_z_lift     = z_lift_val;

        g_sub_step   = 0U;
        g_sub_first  = true;
        g_parked_z   = false;
        g_parked_xy  = false;

        g_heat_start_ms = mc_millis();
        g_phase = FilChgPhase::Heating;

        mc_dbg_434_fchg_state  = 1U;
        mc_dbg_435_fchg_result = 0U;
        mc_dbg_436_fchg_reason = 0U;
        mc_dbg_440_fchg_run_cnt = mc_dbg_440_fchg_run_cnt + 1U;
        mc_dbg_441_fchg_cooled = 0U;
        mc_dbg_442_fchg_wait_s = 0U;
        mc_dbg_443_fchg_beep_cnt = 0U;
    }

    switch (g_phase) {
        case FilChgPhase::Heating: {
            int32_t cur_x10 = 0;
            const bool ok = mc_temp_get_x10(AdcChannel::TempExtruder, &cur_x10);
            if (!ok || mc_time_elapsed_ms(g_heat_start_ms, static_cast<uint32_t>(MC_FILCHG_HEAT_TIMEOUT_S) * 1000U)) {
                return filchg_fail(6U, "M600 heating failed or timeout");
            }
            if (cur_x10 >= (g_saved_target_x10 - (MC_FILCHG_HEAT_BAND_C * 10))) {
                g_phase = FilChgPhase::Retract;
                mc_dbg_434_fchg_state = 2U;
                g_sub_step  = 0U;
                g_sub_first = true;
            }
            return GcodeStatus::Busy;
        }

        case FilChgPhase::Retract: {
            const float e_target = mc_mode_e_relative_get() ? -g_retract_mm : (mc_position_get(kAxisE) - g_retract_mm);
            char line[64];
            uint16_t p = mc_fmt_text(line, sizeof(line), 0U, "G1 E");
            p = mc_fmt_float(line, sizeof(line), p, e_target, 3U);
            p = mc_fmt_text(line, sizeof(line), p, " F");
            p = mc_fmt_uint(line, sizeof(line), p, static_cast<uint32_t>(MC_FILCHG_RETRACT_FEED_MM_MIN));

            const SubMoveRes res = exec_sub_move(line);
            if (res == SubMoveRes::Error) {
                return filchg_fail(5U, "M600 retract move failed");
            }
            if (res == SubMoveRes::Busy) {
                return GcodeStatus::Busy;
            }

            g_phase = FilChgPhase::Park;
            mc_dbg_434_fchg_state = 3U;
            g_sub_step  = 0U;
            g_sub_first = true;
            return GcodeStatus::Busy;
        }

        case FilChgPhase::Park: {
            const uint8_t homed = mc_homing_homed_mask();
            g_parked_z  = (homed & 4U) != 0U;
            g_parked_xy = (homed & 3U) == 3U;

            if (g_sub_step == 0U) {
                if (g_parked_z) {
                    const float cur_z = mc_position_get(kAxisZ);
                    float tgt_z = cur_z + g_z_lift;
                    const float max_z = mc_softlimit_max_get(kAxisZ);
                    if (tgt_z > max_z) { tgt_z = max_z; }

                    if (tgt_z > cur_z) {
                        char line[64];
                        uint16_t p = mc_fmt_text(line, sizeof(line), 0U, "G1 Z");
                        p = mc_fmt_float(line, sizeof(line), p, tgt_z, 3U);
                        p = mc_fmt_text(line, sizeof(line), p, " F");
                        p = mc_fmt_uint(line, sizeof(line), p, static_cast<uint32_t>(MC_FILCHG_Z_FEED_MM_MIN));

                        const SubMoveRes res = exec_sub_move(line);
                        if (res == SubMoveRes::Error) {
                            return filchg_fail(5U, "M600 park Z move failed");
                        }
                        if (res == SubMoveRes::Busy) {
                            return GcodeStatus::Busy;
                        }
                    }
                } else {
                    mc_log(LogLevel::Warn, "M600: park Z skipped, not homed");
                    mc_alert_raise_id(AlertId::FilChgParkSkipped, "Z NOT HOMED");
                }
                g_sub_step  = 1U;
                g_sub_first = true;
            }

            if (g_sub_step == 1U) {
                if (g_parked_xy) {
                    float tgt_x = g_park_x;
                    float tgt_y = g_park_y;

                    const float min_x = mc_softlimit_min_get(kAxisX);
                    const float max_x = mc_softlimit_max_get(kAxisX);
                    const float min_y = mc_softlimit_min_get(kAxisY);
                    const float max_y = mc_softlimit_max_get(kAxisY);

                    if (tgt_x < min_x) { tgt_x = min_x; }
                    if (tgt_x > max_x) { tgt_x = max_x; }
                    if (tgt_y < min_y) { tgt_y = min_y; }
                    if (tgt_y > max_y) { tgt_y = max_y; }

                    char line[64];
                    uint16_t p = mc_fmt_text(line, sizeof(line), 0U, "G1 X");
                    p = mc_fmt_float(line, sizeof(line), p, tgt_x, 3U);
                    p = mc_fmt_text(line, sizeof(line), p, " Y");
                    p = mc_fmt_float(line, sizeof(line), p, tgt_y, 3U);
                    p = mc_fmt_text(line, sizeof(line), p, " F");
                    p = mc_fmt_uint(line, sizeof(line), p, static_cast<uint32_t>(MC_FILCHG_PARK_FEED_MM_MIN));

                    const SubMoveRes res = exec_sub_move(line);
                    if (res == SubMoveRes::Error) {
                        return filchg_fail(5U, "M600 park XY move failed");
                    }
                    if (res == SubMoveRes::Busy) {
                        return GcodeStatus::Busy;
                    }
                } else {
                    mc_log(LogLevel::Warn, "M600: park XY skipped, not homed");
                    mc_alert_raise_id(AlertId::FilChgParkSkipped, "XY NOT HOMED");
                }
            }

            g_phase = FilChgPhase::Unload;
            mc_dbg_434_fchg_state = 4U;
            g_sub_step  = 0U;
            g_sub_first = true;
            return GcodeStatus::Busy;
        }

        case FilChgPhase::Unload: {
            const float e_target = mc_mode_e_relative_get() ? -g_unload_mm : (mc_position_get(kAxisE) - g_unload_mm);
            char line[64];
            uint16_t p = mc_fmt_text(line, sizeof(line), 0U, "G1 E");
            p = mc_fmt_float(line, sizeof(line), p, e_target, 3U);
            p = mc_fmt_text(line, sizeof(line), p, " F");
            p = mc_fmt_uint(line, sizeof(line), p, static_cast<uint32_t>(MC_FILCHG_RETRACT_FEED_MM_MIN));

            const SubMoveRes res = exec_sub_move(line);
            if (res == SubMoveRes::Error) {
                return filchg_fail(5U, "M600 unload move failed");
            }
            if (res == SubMoveRes::Busy) {
                return GcodeStatus::Busy;
            }

            mc_reply_line("echo:Filament change: insert new filament, then press CONTINUE, START key, or send M108");
            g_phase = FilChgPhase::WaitUser;
            mc_dbg_434_fchg_state = 5U;
            g_wait_start_ms     = mc_millis();
            g_last_beep_ms      = mc_millis();
            g_debounce_start_ms = mc_millis();
            g_last_key_raw      = (mc_gpio_read(PinId::KeyStart) == false);
            g_debounced_key     = g_last_key_raw;
            g_key_initial_hold  = g_last_key_raw;
            mc_dbg_444_fchg_start_key = g_debounced_key ? 1U : 0U;
            g_resume_src        = 0U;
            g_resume_requested  = false;
            mc_dbg_441_fchg_cooled   = 0U;
            mc_dbg_442_fchg_wait_s   = 0U;
            mc_dbg_443_fchg_beep_cnt = 0U;
            return GcodeStatus::Busy;
        }

        case FilChgPhase::WaitUser: {
            // ۱) خواندن و debounce کلید START (فعال LOW)
            const bool raw_key = (mc_gpio_read(PinId::KeyStart) == false);
            if (raw_key != g_last_key_raw) {
                g_last_key_raw = raw_key;
                g_debounce_start_ms = mc_millis();
            } else if (mc_time_elapsed_ms(g_debounce_start_ms, MC_FILCHG_KEY_DEBOUNCE_MS)) {
                if (g_debounced_key != raw_key) {
                    g_debounced_key = raw_key;
                    mc_dbg_444_fchg_start_key = g_debounced_key ? 1U : 0U;
                    if (g_debounced_key) {
                        if (!g_key_initial_hold) {
                            mc_filchg_resume_request(2U);
                        }
                    } else {
                        g_key_initial_hold = false;
                    }
                }
            }

            // ۲) بوق دوره‌ای
            const uint32_t beep_period = MC_FILCHG_BEEP_PERIOD_S;
            if ((beep_period > 0U) && mc_time_elapsed_ms(g_last_beep_ms, beep_period * 1000U)) {
                g_last_beep_ms = mc_millis();
                mc_buzzer_click(MC_FILCHG_BEEP_MS);
                mc_dbg_443_fchg_beep_cnt = mc_dbg_443_fchg_beep_cnt + 1U;
            }

            // ۳) خاموش شدن هیتر هنگام timeout
            uint32_t tmo = mc_dbg_446_fchg_tmo_s;
            if (tmo == 0U) { tmo = MC_FILCHG_HEATER_HOLD_S; }
            if ((tmo > 0U) && (mc_dbg_441_fchg_cooled == 0U)) {
                if (mc_time_elapsed_ms(g_wait_start_ms, tmo * 1000U)) {
                    (void)mc_heater_ex_set_target_x10(0);
                    mc_dbg_441_fchg_cooled = 1U;
                    mc_log(LogLevel::Info, "M600: heater hold timeout, hotend off");
                    mc_alert_raise_id(AlertId::FilChgHotendOff);
                }
            }

            // ۴) بررسی درخواست ادامه
            if (g_resume_requested) {
                g_resume_requested = false;
                const uint8_t src = g_resume_src;

                if (mc_filament_empty() && (mc_dbg_445_fchg_ignore_sensor == 0U)) {
                    mc_dbg_439_fchg_resume_refused = mc_dbg_439_fchg_resume_refused + 1U;
                    mc_reply_line("echo:Filament not detected");
                    mc_alert_raise_id(AlertId::FilamentNotFound);
                } else {
                    mc_dbg_438_fchg_resume_cnt = mc_dbg_438_fchg_resume_cnt + 1U;
                    mc_dbg_437_fchg_resume_src = src;

                    if (mc_dbg_441_fchg_cooled == 1U) {
                        (void)mc_heater_ex_set_target_x10(g_saved_target_x10);
                        g_phase = FilChgPhase::Reheat;
                        mc_dbg_434_fchg_state = 6U;
                        g_heat_start_ms = mc_millis();
                    } else {
                        g_phase = FilChgPhase::Load;
                        mc_dbg_434_fchg_state = 7U;
                        g_sub_step  = 0U;
                        g_sub_first = true;
                    }
                }
            }
            return GcodeStatus::Busy;
        }

        case FilChgPhase::Reheat: {
            int32_t cur_x10 = 0;
            const bool ok = mc_temp_get_x10(AdcChannel::TempExtruder, &cur_x10);
            if (!ok || mc_time_elapsed_ms(g_heat_start_ms, static_cast<uint32_t>(MC_FILCHG_HEAT_TIMEOUT_S) * 1000U)) {
                return filchg_fail(6U, "M600 reheat failed or timeout");
            }
            if (cur_x10 >= (g_saved_target_x10 - (MC_FILCHG_HEAT_BAND_C * 10))) {
                g_phase = FilChgPhase::Load;
                mc_dbg_434_fchg_state = 7U;
                g_sub_step  = 0U;
                g_sub_first = true;
            }
            return GcodeStatus::Busy;
        }

        case FilChgPhase::Load: {
            const float e_target = mc_mode_e_relative_get() ? g_load_mm : (mc_position_get(kAxisE) + g_load_mm);
            char line[64];
            uint16_t p = mc_fmt_text(line, sizeof(line), 0U, "G1 E");
            p = mc_fmt_float(line, sizeof(line), p, e_target, 3U);
            p = mc_fmt_text(line, sizeof(line), p, " F");
            p = mc_fmt_uint(line, sizeof(line), p, static_cast<uint32_t>(MC_FILCHG_LOAD_FEED_MM_MIN));

            const SubMoveRes res = exec_sub_move(line);
            if (res == SubMoveRes::Error) {
                return filchg_fail(5U, "M600 load move failed");
            }
            if (res == SubMoveRes::Busy) {
                return GcodeStatus::Busy;
            }

            g_phase = FilChgPhase::Purge;
            mc_dbg_434_fchg_state = 8U;
            g_sub_step  = 0U;
            g_sub_first = true;
            return GcodeStatus::Busy;
        }

        case FilChgPhase::Purge: {
            const float purge_mm = MC_FILCHG_PURGE_MM;
            const float e_target = mc_mode_e_relative_get() ? purge_mm : (mc_position_get(kAxisE) + purge_mm);
            char line[64];
            uint16_t p = mc_fmt_text(line, sizeof(line), 0U, "G1 E");
            p = mc_fmt_float(line, sizeof(line), p, e_target, 3U);
            p = mc_fmt_text(line, sizeof(line), p, " F");
            p = mc_fmt_uint(line, sizeof(line), p, static_cast<uint32_t>(MC_FILCHG_PURGE_FEED_MM_MIN));

            const SubMoveRes res = exec_sub_move(line);
            if (res == SubMoveRes::Error) {
                return filchg_fail(5U, "M600 purge move failed");
            }
            if (res == SubMoveRes::Busy) {
                return GcodeStatus::Busy;
            }

            g_phase = FilChgPhase::Return;
            mc_dbg_434_fchg_state = 9U;
            g_sub_step  = 0U;
            g_sub_first = true;
            return GcodeStatus::Busy;
        }

        case FilChgPhase::Return: {
            if (g_sub_step == 0U) {
                if (g_parked_xy) {
                    char line[64];
                    uint16_t p = mc_fmt_text(line, sizeof(line), 0U, "G1 X");
                    p = mc_fmt_float(line, sizeof(line), p, g_saved_x, 3U);
                    p = mc_fmt_text(line, sizeof(line), p, " Y");
                    p = mc_fmt_float(line, sizeof(line), p, g_saved_y, 3U);
                    p = mc_fmt_text(line, sizeof(line), p, " F");
                    p = mc_fmt_uint(line, sizeof(line), p, static_cast<uint32_t>(MC_FILCHG_PARK_FEED_MM_MIN));

                    const SubMoveRes res = exec_sub_move(line);
                    if (res == SubMoveRes::Error) {
                        return filchg_fail(5U, "M600 return XY move failed");
                    }
                    if (res == SubMoveRes::Busy) {
                        return GcodeStatus::Busy;
                    }
                }
                g_sub_step  = 1U;
                g_sub_first = true;
            }

            if (g_sub_step == 1U) {
                if (g_parked_z) {
                    char line[64];
                    uint16_t p = mc_fmt_text(line, sizeof(line), 0U, "G1 Z");
                    p = mc_fmt_float(line, sizeof(line), p, g_saved_z, 3U);
                    p = mc_fmt_text(line, sizeof(line), p, " F");
                    p = mc_fmt_uint(line, sizeof(line), p, static_cast<uint32_t>(MC_FILCHG_Z_FEED_MM_MIN));

                    const SubMoveRes res = exec_sub_move(line);
                    if (res == SubMoveRes::Error) {
                        return filchg_fail(5U, "M600 return Z move failed");
                    }
                    if (res == SubMoveRes::Busy) {
                        return GcodeStatus::Busy;
                    }
                }
            }

            return filchg_success();
        }

        case FilChgPhase::Idle:
        default:
            return filchg_fail(7U, "M600 unknown phase");
    }
}

}  // namespace mc
