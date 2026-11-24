#pragma once
// صفحه SD CARD رابط کاربری (بند ۸.۱۳).
// فقط از هدرهای core و hal استفاده می‌کند. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// متغیرهای دیباگ صفحه SD CARD (بند ۸.۱۳)
extern volatile uint32_t mc_dbg_349_sd_sel_shown;   // خواندنی: اندیس مورد فوکوس‌شده (۰ تا ۲)
extern volatile uint32_t mc_dbg_350_sd_enter_cnt;   // خواندنی: تعداد اینترها روی مورد غیر BACK
extern volatile uint32_t mc_dbg_351_sd_enter_idx;   // خواندنی: اندیس مورد آخرین اینتر (۲۵۵ = هنوز نشده)
extern volatile uint32_t mc_dbg_352_sd_force;       // نوشتنی: ۰ = بی‌اثر؛ ۱ = باز کردن صفحه؛ ۲ = برگشت به منو؛ ۳ تا ۵ = فوکوس روی مورد (مقدار - ۳)
extern volatile uint32_t mc_dbg_353_sd_fake;        // نوشتنی: حالت ساختگی ظاهر صفحه SD CARD (بیت ۷ = فعال‌سازی)

#ifdef __cplusplus
}

namespace mc {

enum class SdResult : uint8_t {
    Stay = 0,
    Back = 1
};

// شروع نقاشی مرحله‌ای ورود و دور ریختن کلیک‌ها و اینترهای قدیمی.
void mc_ui_sd_open();

// وظیفه زمان‌بند صفحه SD CARD: در هر دور یک تکه کوچک نقاشی کرده یا ورودی انکودر را می‌خواند.
SdResult mc_ui_sd_poll();

}  // namespace mc
#endif
