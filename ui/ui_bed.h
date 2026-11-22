#pragma once
// صفحه داغ رابط کاربری (بند ۸.۱۰).
// فقط از هدرهای core و hal استفاده می‌کند. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// متغیرهای دیباگ صفحه داغ (بند ۸.۱۰)
extern volatile uint32_t mc_dbg_340_bed_sel_shown;   // خواندنی: اندیس مورد فوکوس‌شده (۰ تا ۳)
extern volatile uint32_t mc_dbg_341_bed_enter_cnt;   // خواندنی: تعداد اینترها روی مورد غیر BACK
extern volatile uint32_t mc_dbg_342_bed_enter_idx;   // خواندنی: اندیس مورد آخرین اینتر (۲۵۵ = هنوز نشده)
extern volatile uint32_t mc_dbg_343_bed_force;       // نوشتنی: ۰ = بی‌اثر؛ ۱ = باز کردن صفحه داغ؛ ۲ = برگشت به منو؛ ۳ تا ۶ = فوکوس روی مورد (مقدار - ۳)

#ifdef __cplusplus
}

namespace mc {

enum class BedResult : uint8_t {
    Stay = 0,
    Back = 1
};

// شروع نقاشی مرحله‌ای ورود و دور ریختن کلیک‌ها و اینترهای قدیمی.
void mc_ui_bed_open();

// وظیفه زمان‌بند صفحه داغ: در هر دور یک تکه کوچک نقاشی کرده یا ورودی انکودر را می‌خواند.
BedResult mc_ui_bed_poll();

}  // namespace mc
#endif
