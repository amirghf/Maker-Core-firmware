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

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile int mc_dbg_07_killed       = 0;  // 1 = برد در حالت kill است
volatile int mc_dbg_08_error_code   = 0;  // آخرین کد خطا
volatile int mc_dbg_10_kill_safe_ok = -1; // بعد از kill: 1 = همه خروجی‌ها امن بودند، 0 = نبودند، -1 = kill نشده
}

namespace mc {
namespace {

volatile bool      g_killed = false;
volatile ErrorCode g_error  = ErrorCode::None;

constexpr uint32_t kReportMs    = 3000U;  // هر ۳ ثانیه یک‌بار خطا اعلام شود
constexpr uint32_t kSafeStateMs = 100U;   // هر ۱۰۰ میلی‌ثانیه حالت امن دوباره اعمال شود

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

    // اول از همه همه خروجی‌ها را خاموش کن و بررسی کن واقعاً امن شده‌اند
    mc_board_safe_state();
    mc_dbg_10_kill_safe_ok = mc_board_safe_state_check() ? 1 : 0;

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
    if (v >= 10U) { code_str[cn++] = static_cast<char>('0' + ((v % 100U) / 10U)); }
    code_str[cn++] = static_cast<char>('0' + (v % 10U));
    code_str[cn] = '\0';

    mc_gfx_text(30U, 70U, code_str, kColorWhite, mc_rgb565(20, 24, 33), 2U);
    mc_gfx_text(30U, 100U, "SYSTEM HALTED FOR SAFETY", kErrAcc, mc_rgb565(20, 24, 33), 2U);
    mc_gfx_text(30U, 130U, "RESET REQUIRED TO RESUME", kColorWhite, mc_rgb565(20, 24, 33), 2U);

    // دکمه OK برای ساکت کردن بازر
    mc_gfx_fill_rect(170U, 240U, 140U, 48U, mc_rgb565(24, 38, 64));
    mc_gfx_rect(170U, 240U, 140U, 48U, kErrAcc);
    const uint16_t ok_w = mc_gfx_text_width("OK", 2U);
    mc_gfx_text(static_cast<uint16_t>(170U + (140U - ok_w) / 2U), 256U, "OK", kColorWhite, mc_rgb565(24, 38, 64), 2U);

    // اشاره برای کاربر
    const char* help_txt = "PRESS OK OR ENCODER TO SILENCE";
    const uint16_t hw = mc_gfx_text_width(help_txt, 1U);
    mc_gfx_text(static_cast<uint16_t>((480U - hw) / 2U), 300U, help_txt, mc_rgb565(150, 160, 180), kColorBlack, 1U);

    // حلقه امن: تا ریست دستی در این حلقه می‌مانیم
    uint32_t last_safe   = mc_millis();
    uint32_t last_report = mc_millis() - kReportMs;  // اولین اعلام فوراً انجام شود
    bool buzzer_silenced = false;

    for (;;) {
        mc_watchdog_feed();  // برد خودش ریست نشود و خطا پنهان نماند

        // پایش کلید انکودر یا لمس برای ساکت‌کردن بازر
        if (!buzzer_silenced) {
            bool btn_pressed = !mc_gpio_read(PinId::EncBtn);
            if (!btn_pressed && mc_touch_pressed()) {
                uint16_t tx = 0U, ty = 0U;
                if (mc_touch_read_raw(&tx, &ty)) {
                    (void)tx;
                    (void)ty;
                    btn_pressed = true;
                }
            }
            if (btn_pressed) {
                buzzer_silenced = true;
                mc_buzzer_off();
                mc_dbg_484_alert_kill_screen = 2U; // ساکت شد
            }
        }

        if (mc_time_elapsed_ms(last_safe, kSafeStateMs)) {
            mc_board_safe_state();
            last_safe = mc_millis();
        }
        if (mc_time_elapsed_ms(last_report, kReportMs)) {
            last_report = mc_millis();
            report_serial(code);
            if (!buzzer_silenced) {
                mc_buzzer_play(Sound::Error);
            }
            mc_watchdog_feed();
        }
    }
}

}  // namespace mc
