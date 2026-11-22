#pragma once
// صفحه اکسترودر رابط کاربری (بند ۸.۹).
// فقط از هدرهای core و hal استفاده می‌کند. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// متغیرهای دیباگ صفحه اکسترودر (بند ۸.۹)
extern volatile uint32_t mc_dbg_334_ex_sel_shown;   // خواندنی: اندیس مورد فوکوس‌شده (۰ تا ۷)
extern volatile uint32_t mc_dbg_335_ex_enter_cnt;   // خواندنی: تعداد اینترها روی مورد غیر BACK و غیر چیپ‌ها
extern volatile uint32_t mc_dbg_336_ex_enter_idx;   // خواندنی: اندیس مورد آخرین اینتر (۲۵۵ = هنوز نشده)
extern volatile uint32_t mc_dbg_337_ex_force;       // نوشتنی: ۰ = بی‌اثر؛ ۱ = باز کردن صفحه اکسترودر؛ ۲ = برگشت به منو؛ ۳ تا ۱۰ = فوکوس روی مورد (مقدار - ۳)
extern volatile uint32_t mc_dbg_338_ex_feed_shown;  // خواندنی: اندیس چیپ گام نمایشی (۰ تا ۲)
extern volatile uint32_t mc_dbg_339_ex_fake_rule;   // نوشتنی: ۰ = وضعیت واقعی HOT RULE؛ ۱ = نمایش «روشن»؛ ۲ = نمایش «خاموش»

#ifdef __cplusplus
}

namespace mc {

enum class ExtruderResult : uint8_t {
    Stay = 0,
    Back = 1
};

// شروع نقاشی مرحله‌ای ورود و دور ریختن کلیک‌ها و اینترهای قدیمی.
void mc_ui_extruder_open();

// وظیفه زمان‌بند صفحه اکسترودر: در هر دور یک تکه کوچک نقاشی کرده یا ورودی انکودر را می‌خواند.
ExtruderResult mc_ui_extruder_poll();

}  // namespace mc
#endif
