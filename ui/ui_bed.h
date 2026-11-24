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

// متغیرهای دیباگ عملکردهای صفحه داغ (بند ۸.۱۶)
extern volatile uint32_t mc_dbg_384_bed_act_cnt;        // خواندنی: تعداد دفعاتی که دکمه‌ها واقعاً هدف هیتر بستر را عوض کردند
extern volatile uint32_t mc_dbg_385_bed_act_last_c;     // خواندنی: آخرین هدف (درجه سانتی‌گراد) که دکمه‌ها گذاشتند
extern volatile uint32_t mc_dbg_386_bed_act_refused;    // خواندنی: تعداد دفعاتی که دکمه به‌خاطر شرط ایمنی رد شد
extern volatile uint32_t mc_dbg_387_bed_act_en;         // نوشتنی: 0 = دکمه‌ها هیتر را عوض نمی‌کنند (پیش‌فرض 1)
extern volatile uint32_t mc_dbg_388_bed_refuse_reason;  // خواندنی: علت آخرین رد شدن (0 = ندارد، 1 = حالت تست هیتر، 2 = kill، 3 = حالت fake روشن، 4 = کلید 387 صفر)

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
