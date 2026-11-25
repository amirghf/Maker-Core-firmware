#pragma once
// صفحه اصلی رابط کاربری (بند ۸.۲).
// فقط از هدرهای core و hal استفاده می‌کند. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "../config/mc_config.h"

#ifdef __cplusplus
extern "C" {
#endif

extern volatile uint32_t mc_dbg_297_ui_fake_on;         // نوشتنی: ۱ = مقدارهای ساختگی دما، هدف و وضعیت به‌جای واقعی
extern volatile int32_t  mc_dbg_298_ui_fake_noz_x10;     // نوشتنی: دمای فعلی اکسترودر ×۱۰ (فقط وقتی fake روشن است). -9999 = سنسور خراب (بند ۸.۹)
extern volatile int32_t  mc_dbg_299_ui_fake_bed_x10;     // نوشتنی: دمای فعلی بستر ×۱۰ (فقط وقتی fake روشن است). -9999 = سنسور خراب (بند ۸.۱۰)
extern volatile int32_t  mc_dbg_300_ui_fake_noz_tgt_x10; // نوشتنی: هدف اکسترودر ×۱۰ (بند ۸.۹)
extern volatile int32_t  mc_dbg_301_ui_fake_bed_tgt_x10; // نوشتنی: هدف بستر ×۱۰ (بند ۸.۱۰)
extern volatile uint32_t mc_dbg_304_ui_fake_fan_pct;      // نوشتنی: درصد فن ۰ تا ۱۰۰ (فقط وقتی fake روشن است) (بند ۸.۹)
extern volatile uint32_t mc_dbg_305_ui_fake_axis_en;     // نوشتنی: بیت ۰ تا ۳ = فعال بودن X، Y، Z، E (فقط وقتی fake روشن است)
extern volatile uint32_t mc_dbg_323_ui_page;            // خواندنی: ۰ = صفحه اصلی، ۱ = منو (بند ۸.۷)، ۲ = صفحه حرکت (بند ۸.۸)، ۳ = صفحه اکسترودر (بند ۸.۹)، ۴ = صفحه داغ (بند ۸.۱۰)، ۵ = صفحه سنسورها (بند ۸.۱۲)، ۶ = صفحه SD CARD (بند ۸.۱۳)، ۷ = صفحه USB OTG (بند ۸.۱۴)، ۸ = صفحه ABOUT (بند ۸.۱۸)، ۹ = صفحه TOUCH CAL (بند ۸.۱۵)، ۱۰ = صفحه DISPLAY (بند ۸.۲۱)، ۱۱ = صفحه FILAMENT CHANGE (بند ۹.۱)، ۱۲ = صفحه MESSAGE (بند ۱۰.۱)
extern volatile uint32_t mc_dbg_359_ui_fake_print_pct;   // نوشتنی: درصد ساختگی چاپ ۰ تا ۱۰۰ (فقط وقتی fake روشن است) (بند ۸.۶)

// متغیرهای دیباگ عملکردهای صفحه اصلی (بند ۸.۱۶.۵)
extern volatile uint32_t mc_dbg_409_home_act_en;       // نوشتنی: 0 = دکمه‌های START و PAUSE و STOP و RESET هیچ کاری نمی‌کنند (پیش‌فرض 1)
extern volatile uint32_t mc_dbg_410_home_act_cnt;      // خواندنی: تعداد دفعاتی که دکمه‌ها واقعاً کاری انجام دادند
extern volatile uint32_t mc_dbg_411_home_act_last;     // خواندنی: اندیس آخرین دکمه موفق (0 = START، 1 = PAUSE، 2 = STOP، 3 = RESET؛ 255 = هنوز نشده)
extern volatile uint32_t mc_dbg_412_home_act_refused;  // خواندنی: تعداد دفعاتی که دکمه رد شد
extern volatile uint32_t mc_dbg_413_home_refuse_reason;// خواندنی: علت آخرین رد شدن
extern volatile uint32_t mc_dbg_414_home_reset_dry;    // نوشتنی: 1 = دکمه RESET همه کار را می‌کند ولی ریست سخت‌افزاری نمی‌زند (برای تست)؛ 0 = ریست واقعی (پیش‌فرض 0)
extern volatile uint32_t mc_dbg_415_home_moving_mask;  // خواندنی: بیت 0 تا 3 = حرکت واقعی (یا fake) محور X، Y، Z، E
extern volatile uint32_t mc_dbg_416_home_fake_move;    // نوشتنی: بیت 0 تا 3 = حرکت ساختگی X، Y، Z، E
// متغیرهای mc_dbg_362/370/371 (TOUCH CAL) در ui_tcal.h اعلان شده‌اند (بند ۸.۱۵).


#ifdef __cplusplus
}

namespace mc {

constexpr uint32_t kUiPollMs = MC_UI_POLL_MS;

// بند ۸.۱۵: وضعیت فشرده بودن دکمه انکودر در لحظه روشن شدن دستگاه
// (هسته آن را در mc_core_init_c پر می‌کند و UI برای ورود به صفحه TOUCH CAL می‌خواند).
extern bool g_enc_pressed_at_boot;

// راه‌اندازی ماژول صفحه اصلی UI.
void mc_ui_init();

// وظیفه زمان‌بند صفحه اصلی UI (هر kUiPollMs میلی‌ثانیه).
// در هر دور فقط یک مرحله از قاب ثابت یا یک فیلد متغیر را نقاشی می‌کند.
void mc_ui_poll();

// هسته بعد از هر بار نمایش اسپلش این تابع را صدا می‌زند تا نتیجه اسپلش به UI اعلام شود.
void mc_ui_notify_splash(uint32_t result);

}  // namespace mc
#endif
