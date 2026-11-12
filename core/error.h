#pragma once
// سیستم خطا و kill. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

namespace mc {

// کدهای خطا. کدهای جدید در مراحل بعدی به انتها اضافه می‌شوند؛ شماره‌های قبلی را عوض نکن.
enum class ErrorCode : uint8_t {
    None           = 0,
    Test           = 1,   // خطای آزمایشی برای تست kill
    SelfTestFailed = 2,   // یک سلف‌تست هنگام راه‌اندازی رد شد
    Internal       = 3    // خطای داخلی برنامه
};

// برد را متوقف می‌کند: همه خروجی‌ها را در حالت امن می‌گذارد و تا ریست دستی در حلقه امن می‌ماند.
// هرگز برنمی‌گردد. در حلقه، Watchdog تغذیه می‌شود و پیام خطا با بوق و USB اعلام می‌شود.
[[noreturn]] void mc_kill(ErrorCode code);

// آیا برد در حالت kill است؟ (برای کدهایی که از وقفه صدا می‌زنند)
bool mc_is_killed();

// آخرین کد خطا (None اگر خطایی نبوده).
ErrorCode mc_last_error();

}  // namespace mc
