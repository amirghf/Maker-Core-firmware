#pragma once
// صفحه ABOUT رابط کاربری (بند ۸.۱۸).
// فقط از هدرهای core و hal استفاده می‌کند. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// متغیرهای دیباگ صفحه ABOUT (بند ۸.۱۸)
extern volatile uint32_t mc_dbg_360_about_sel_shown; // خواندنی: اندیس مورد فوکوس‌شده (همیشه ۰ = BACK)
extern volatile uint32_t mc_dbg_361_about_force;     // نوشتنی: ۰ = بی‌اثر؛ ۱ = باز کردن صفحه؛ ۲ = برگشت به منو

#ifdef __cplusplus
}

namespace mc {

enum class AboutResult : uint8_t {
    Stay = 0,
    Back = 1
};

// شروع نقاشی مرحله‌ای ورود و دور ریختن کلیک‌ها و اینترهای قدیمی.
void mc_ui_about_open();

// وظیفه زمان‌بند صفحه ABOUT: در هر دور یک تکه کوچک نقاشی کرده یا ورودی انکودر را می‌خواند.
AboutResult mc_ui_about_poll();

}  // namespace mc
#endif
