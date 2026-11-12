// سیستم خطا و kill. هیچ هدر ST اینجا include نمی‌شود.
#include "error.h"
#include "hal_board.h"
#include "hal_watchdog.h"
#include "hal_buzzer.h"
#include "hal_serial.h"
#include "hal_time.h"

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile int mc_dbg_killed        = 0;  // 1 = برد در حالت kill است
volatile int mc_dbg_error_code    = 0;  // آخرین کد خطا
volatile int mc_dbg_kill_safe_ok  = -1; // بعد از kill: 1 = همه خروجی‌ها امن بودند، 0 = نبودند، -1 = kill نشده
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
    mc_dbg_error_code = static_cast<int>(code);
    mc_dbg_killed = 1;

    // اول از همه همه خروجی‌ها را خاموش کن و بررسی کن واقعاً امن شده‌اند
    mc_board_safe_state();
    mc_dbg_kill_safe_ok = mc_board_safe_state_check() ? 1 : 0;

    // حلقه امن: تا ریست دستی اینجا می‌مانیم
    uint32_t last_safe   = mc_millis();
    uint32_t last_report = mc_millis() - kReportMs;  // اولین اعلام فوراً انجام شود
    for (;;) {
        mc_watchdog_feed();  // برد خودش ریست نشود و خطا پنهان نماند
        if (mc_time_elapsed_ms(last_safe, kSafeStateMs)) {
            mc_board_safe_state();
            last_safe = mc_millis();
        }
        if (mc_time_elapsed_ms(last_report, kReportMs)) {
            last_report = mc_millis();
            report_serial(code);
            mc_buzzer_play(Sound::Error);
            mc_watchdog_feed();
        }
    }
}

}  // namespace mc
