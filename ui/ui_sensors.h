#pragma once
// صفحه سنسورها رابط کاربری (بند ۸.۱۲).
// فقط از هدرهای core و hal استفاده می‌کند. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// متغیرهای دیباگ صفحه سنسورها (بند ۸.۱۲)
extern volatile uint32_t mc_dbg_344_sen_sel_shown;   // خواندنی: اندیس مورد فوکوس‌شده (۰ تا ۴)
extern volatile uint32_t mc_dbg_345_sen_enter_cnt;   // خواندنی: تعداد اینترها روی مورد غیر BACK
extern volatile uint32_t mc_dbg_346_sen_enter_idx;   // خواندنی: اندیس مورد آخرین اینتر (۲۵۵ = هنوز نشده)
extern volatile uint32_t mc_dbg_347_sen_force;       // نوشتنی: ۰ = بی‌اثر؛ ۱ = باز کردن صفحه سنسورها؛ ۲ = برگشت به منو؛ ۳ تا ۷ = فوکوس روی مورد (مقدار - ۳)
extern volatile uint32_t mc_dbg_348_sen_fake;        // نوشتنی: حالت ساختگی ظاهر صفحه سنسورها، بیت‌ها طبق بند پ-۱۲ (بیت ۷ = فعال‌سازی)

#ifdef __cplusplus
}

namespace mc {

enum class SensorsResult : uint8_t {
    Stay = 0,
    Back = 1
};

// شروع نقاشی مرحله‌ای ورود و دور ریختن کلیک‌ها و اینترهای قدیمی.
void mc_ui_sensors_open();

// وظیفه زمان‌بند صفحه سنسورها: در هر دور یک تکه کوچک نقاشی کرده یا ورودی انکودر را می‌خواند.
SensorsResult mc_ui_sensors_poll();

}  // namespace mc
#endif
