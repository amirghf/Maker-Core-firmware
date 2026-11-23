#pragma once
// صفحه USB OTG رابط کاربری (بند ۸.۱۵).
// فقط از هدرهای core و hal استفاده می‌کند. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// متغیرهای دیباگ صفحه USB OTG (بند ۸.۱۵)
extern volatile uint32_t mc_dbg_354_otg_sel_shown;   // خواندنی: اندیس مورد فوکوس‌شده (۰ تا ۲)
extern volatile uint32_t mc_dbg_355_otg_enter_cnt;   // خواندنی: تعداد اینترها روی مورد غیر BACK
extern volatile uint32_t mc_dbg_356_otg_enter_idx;   // خواندنی: اندیس مورد آخرین اینتر (۲۵۵ = هنوز نشده)
extern volatile uint32_t mc_dbg_357_otg_force;       // نوشتنی: ۰ = بی‌اثر؛ ۱ = باز کردن صفحه؛ ۲ = برگشت به منو؛ ۳ تا ۵ = فوکوس روی مورد (مقدار - ۳)
extern volatile uint32_t mc_dbg_358_otg_fake;        // نوشتنی: حالت ساختگی ظاهر صفحه USB OTG (بیت ۷ = فعال‌سازی)

#ifdef __cplusplus
}

namespace mc {

enum class OtgResult : uint8_t {
    Stay = 0,
    Back = 1
};

// شروع نقاشی مرحله‌ای ورود و دور ریختن کلیک‌ها و اینترهای قدیمی.
void mc_ui_otg_open();

// وظیفه زمان‌بند صفحه USB OTG: در هر دور یک تکه کوچک نقاشی کرده یا ورودی انکودر را می‌خواند.
OtgResult mc_ui_otg_poll();

}  // namespace mc
#endif
