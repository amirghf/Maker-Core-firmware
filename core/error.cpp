// سیستم خطا و kill. هیچ هدر ST اینجا include نمی‌شود.
#include "error.h"
#include "hal_board.h"
#include "hal_watchdog.h"
#include "hal_buzzer.h"
#include "hal_serial.h"
#include "hal_time.h"
#include "hal_display.h"
#include "gfx.h"
#include "alert.h"
#include "hal_gpio.h"
#include "pins.h"
#include "hal_touch.h"
#include "touch.h"
#include "../config/mc_config.h"

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile int mc_dbg_07_killed       = 0;  // 1 = برد در حالت kill است
volatile int mc_dbg_08_error_code   = 0;  // آخرین کد خطا
volatile int mc_dbg_10_kill_safe_ok = -1; // بعد از kill: 1 = همه خروجی‌ها امن بودند، 0 = نبودند، -1 = kill نشده
volatile uint32_t mc_dbg_490_kill_bl_cnt = 0U; // خواندنی: تعداد دفعاتی که بک‌لایت در مسیر kill روشن شد (بند ۱۰.۱۴)
volatile uint32_t mc_dbg_491_kill_ok_state = 0U; // خواندنی: 0 = OK هنوز نخورده، 1 = OK قبول شد و در حال ریست، 2 = OK قبول شد ولی ریست آزمایشی بود (بند ۱۰.۱۴)
volatile uint32_t mc_dbg_492_kill_reset_dry = 0U; // نوشتنی: 0 = ریست واقعی (پیش‌فرض)، 1 = ریست آزمایشی (ریست نمی‌شود) (بند ۱۰.۱۴)
}

namespace mc {
namespace {

volatile bool      g_killed = false;
volatile ErrorCode g_error  = ErrorCode::None;

constexpr uint32_t kReportMs    = 3000U;  // هر ۳ ثانیه یک‌بار خطا اعلام شود
constexpr uint32_t kSafeStateMs = 100U;   // هر ۱۰۰ میلی‌ثانیه حالت امن دوباره اعمال شود

// تبدیل کد خطا به نام علت برای نمایش روی کارت (بند ۱۰.۱۴)
const char* error_code_to_name(ErrorCode code) {
    switch (code) {
        case ErrorCode::Test:               return "TEST ERROR";
        case ErrorCode::SelfTestFailed:     return "SELF TEST FAILED";
        case ErrorCode::Internal:           return "INTERNAL ERROR";
        case ErrorCode::MinTempExtruder:    return "EXTRUDER MIN TEMP";
        case ErrorCode::MaxTempExtruder:    return "EXTRUDER MAX TEMP";
        case ErrorCode::MinTempBed:         return "BED MIN TEMP";
        case ErrorCode::MaxTempBed:          return "BED MAX TEMP";
        case ErrorCode::WatchHeatingEx:     return "EXTRUDER WATCH HEATING";
        case ErrorCode::WatchHeatingBed:    return "BED WATCH HEATING";
        case ErrorCode::ThermalRunawayEx:   return "EXTRUDER THERMAL RUNAWAY";
        case ErrorCode::ThermalRunawayBed:  return "BED THERMAL RUNAWAY";
        case ErrorCode::HeaterFailedEx:     return "EXTRUDER HEATER FAILED";
        case ErrorCode::HeaterFailedBed:    return "BED HEATER FAILED";
        case ErrorCode::EndstopDuringPrint: return "ENDSTOP DURING PRINT";
        case ErrorCode::EmergencyStop:      return "EMERGENCY STOP (M112)";
        default:                            return "UNKNOWN ERROR";
    }
}

// پیام خطا را برای کامپیوتر می‌فرستد (فقط اگر پورت باز باشد).
void report_serial(ErrorCode code) {
    if (!mc_serial_connected()) { return; }
    char msg[48];
    const char* prefix = "Error: kill() called, code=";
    uint8_t n = 0;
    while (prefix[n] != '\0') {
        msg[n] = prefix[n];
        n = static_cast<uint8_t>(n + 1U);
    }
    const uint8_t v = static_cast<uint8_t>(code);
    const uint8_t hundreds = static_cast<uint8_t>(v / 100U);
    const uint8_t tens     = static_cast<uint8_t>((v % 100U) / 10U);
    const uint8_t ones     = static_cast<uint8_t>(v % 10U);
    if (hundreds != 0U) { msg[n++] = static_cast<char>('0' + hundreds); }
    if ((hundreds != 0U) || (tens != 0U)) { msg[n++] = static_cast<char>('0' + tens); }
    msg[n++] = static_cast<char>('0' + ones);
    msg[n++] = '\r';
    msg[n++] = '\n';
    msg[n] = '\0';
    (void)mc_serial_print(msg);
}

}  // namespace

bool mc_is_killed() {
    return g_killed;
}

ErrorCode mc_last_error() {
    return g_error;
}

[[noreturn]] void mc_kill(ErrorCode code) {
    g_error  = code;
    g_killed = true;
    mc_dbg_08_error_code = static_cast<int>(code);
    mc_dbg_07_killed     = 1;
    mc_dbg_491_kill_ok_state = 0U;

    // اول از همه همه خروجی‌ها را خاموش کن و بررسی کن واقعاً امن شده‌اند
    mc_board_safe_state();
    mc_dbg_10_kill_safe_ok = mc_board_safe_state_check() ? 1 : 0;

    // بند ۱۰.۱۴: بک‌لایت بعد از حالت امن دوباره روشن می‌شود تا صفحه kill دیده شود
    mc_display_backlight(true);
    mc_dbg_490_kill_bl_cnt = mc_dbg_490_kill_bl_cnt + 1U;

    // بند ۱۰.۱۳: نمایش صفحه kill تمام‌صفحه
    mc_dbg_484_alert_kill_screen = 1U;

    mc_display_fill_screen(kColorBlack);
    constexpr uint16_t kErrBar = mc_rgb565(150, 25, 25);
    constexpr uint16_t kErrAcc = mc_rgb565(240, 70, 70);
    mc_display_fill_rect(0U, 0U, 480U, 36U, kErrBar);
    mc_display_fill_rect(0U, 36U, 480U, 2U, kErrAcc);
    const uint16_t tw = mc_gfx_text_width("SYSTEM KILLED - ERROR", 2U);
    mc_gfx_text(static_cast<uint16_t>((480U - tw) / 2U), 10U, "SYSTEM KILLED - ERROR", kColorWhite, kErrBar, 2U);

    // کارت خطا
    mc_gfx_fill_rect(14U, 50U, 452U, 172U, mc_rgb565(20, 24, 33));
    mc_gfx_rect(14U, 50U, 452U, 172U, kErrAcc);
    mc_gfx_fill_rect(14U, 50U, 4U, 172U, kErrAcc);

    char code_str[48];
    const char* p1 = "ERROR CODE: ";
    uint8_t cn = 0U;
    while (*p1 != '\0') { code_str[cn++] = *p1++; }
    const uint8_t v = static_cast<uint8_t>(code);
    if (v >= 100U) { code_str[cn++] = static_cast<char>('0' + (v / 100U)); }
    if (v >= 10U) { code_str[cn++] = static_str_cast<char>('0' + ((v % 100U) / 10U)); }
    code_str[cn++] = static_cast<char>('0' + (v % 10U));
    code_str[cn] = '\0';

    // خط ۱: شماره کد خطا (y=70)
    mc_gfx_text(30U, 70U, code_str, kColorWhite, mc_rgb565(20, 24, 33), 2U);
    // خط ۲: نام علت خطا (y=100)
    mc_gfx_text(30U, 100U, error_code_to_name(code), kColorWhite, mc_rgb565(20, 24, 33), 2U);
    // خط ۳: هشدار ایمنی (y=130)
    mc_gfx_text(30U, 130U, "SYSTEM HALTED FOR SAFETY", kErrAcc, mc_rgb565(20, 24, 33), 2U);
    // خط ۴: راهنمای ریست (y=160)
    mc_gfx_text(30U, 160U, "PRESS OK TO RESTART BOARD", kColorWhite, mc_rgb565(20, 24, 33), 2U);

    // دکمه OK برای ریست خودکار برد
    mc_gfx_fill_rect(170U, 240U, 140U, 48U, mc_rgb565(24, 38, 64));
    mc_gfx_rect(170U, 240U, 140U, 48U, kErrAcc);
    const uint16_t ok_w = mc_gfx_text_width("OK", 2U);
    mc_gfx_text(static_cast<uint16_t>(170U + (140U - ok_w) / 2U), 256U, "OK", kColorWhite, mc_rgb565(24, 38, 64), 2U);

    // متن ریز زیر دکمه (y=300, scale 1)
    const char* help_txt = "PRESS OK OR ENCODER TO RESTART";
    const uint16_t hw = mc_gfx_text_width(help_txt, 1U);
    mc_gfx_text(static_cast<uint16_t>((480U - hw) / 2U), 300U, help_txt, mc_rgb565(150, 160, 180), kColorBlack, 1U);

    const uint32_t screen_rendered_ms = mc_millis();
    bool armed = false;
    bool ok_accepted = false;
    uint32_t ok_accepted_ms = 0U;

    uint32_t last_safe   = mc_millis();
    uint32_t last_report = mc_millis() - kReportMs;  // اولین اعلام فوراً انجام شود

    for (;;) {
        mc_watchdog_feed();  // برد خودش ریست نشود و خطا پنهان نماند

        const uint32_t now = mc_millis();

        // مسلح شدن: گذر زمان MC_KILL_OK_GUARD_MS و رها شدن انکودر و لمس
        if (!armed) {
            const bool guard_passed = mc_time_elapsed_ms(screen_rendered_ms, MC_KILL_OK_GUARD_MS);
            const bool btn_down     = !mc_gpio_read(PinId::EncBtn);
            const bool touch_down   = mc_touch_pressed();
            if (guard_passed && !btn_down && !touch_down) {
                armed = true;
            }
        }

        // بررسی ورود OK پس از مسلح شدن
        if (armed && !ok_accepted) {
            bool accept = false;
            if (!mc_gpio_read(PinId::EncBtn)) {
                accept = true;
            } else if (mc_touch_pressed()) {
                uint16_t tx = 0U, ty = 0U;
                if (mc_touch_read_raw(&tx, &ty)) {
                    if (mc_touch_is_calibrated()) {
                        uint16_t sx = 0U, sy = 0U;
                        if (mc_touch_apply(tx, ty, &sx, &sy)) {
                            if ((sx >= 170U) && (sx < (170U + 140U)) &&
                                (sy >= 240U) && (sy < (240U + 48U))) {
                                accept = true;
                            }
                        }
                    } else {
                        accept = true;
                    }
                }
            }

            if (accept) {
                ok_accepted = true;
                ok_accepted_ms = now;
                mc_buzzer_off();
                mc_dbg_484_alert_kill_screen = 3U;
                mc_dbg_491_kill_ok_state = 1U;

                // پاک‌کردن دکمه OK و نوشتن RESTARTING...
                mc_gfx_fill_rect(170U, 240U, 140U, 48U, mc_rgb565(24, 38, 64));
                mc_gfx_rect(170U, 240U, 140U, 48U, kErrAcc);
                const char* rst_txt = "RESTARTING...";
                const uint16_t rst_w = mc_gfx_text_width(rst_txt, 2U);
                const uint16_t rst_x = static_cast<uint16_t>(170U + (140U - rst_w) / 2U);
                mc_gfx_text(rst_x, 256U, rst_txt, kColorWhite, mc_rgb565(24, 38, 64), 2U);
            }
        }

        // پس از قبول OK و گذر زمان تأخیر ریست
        if (ok_accepted && (mc_dbg_491_kill_ok_state == 1U)) {
            if (mc_time_elapsed_ms(ok_accepted_ms, MC_KILL_RESTART_DELAY_MS)) {
                if (mc_dbg_492_kill_reset_dry == 0U) {
                    mc_system_reset();  // ریست سخت‌افزاری (برنمی‌گردد)
                } else {
                    mc_dbg_491_kill_ok_state = 2U;  // ریست آزمایشی (بدون ریست در حلقه می‌ماند)
                }
            }
        }

        if (mc_time_elapsed_ms(last_safe, kSafeStateMs)) {
            mc_board_safe_state();
            mc_display_backlight(true);
            mc_dbg_490_kill_bl_cnt = mc_dbg_490_kill_bl_cnt + 1U;
            last_safe = mc_millis();
        }

        if (mc_time_elapsed_ms(last_report, kReportMs)) {
            last_report = mc_millis();
            report_serial(code);
            if (!ok_accepted) {
                mc_buzzer_play(Sound::Error);
            }
            mc_watchdog_feed();
        }
    }
}

}  // namespace mc
