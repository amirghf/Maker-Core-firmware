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
extern volatile uint32_t mc_dbg_323_ui_page;            // خواندنی: ۰ = صفحه اصلی، ۱ = منو (بند ۸.۷)، ۲ = صفحه حرکت (بند ۸.۸)، ۳ = صفحه اکسترودر (بند ۸.۹)، ۴ = صفحه داغ (بند ۸.۱۰)، ۵ = صفحه سنسورها (بند ۸.۱۲)، ۶ = صفحه SD CARD (بند ۸.۱۴)، ۷ = صفحه USB OTG (بند ۸.۱۵)، ۸ = صفحه ABOUT (بند ۸.۲۰)، ۹ = صفحه TOUCH CAL (بند ۸.۱۷)
extern volatile uint32_t mc_dbg_359_ui_fake_print_pct;   // نوشتنی: درصد ساختگی چاپ ۰ تا ۱۰۰ (فقط وقتی fake روشن است) (بند ۸.۶)
// متغیرهای mc_dbg_362/370/371 (TOUCH CAL) در ui_tcal.h اعلان شده‌اند (بند ۸.۱۷).


#ifdef __cplusplus
}

namespace mc {

constexpr uint32_t kUiPollMs = MC_UI_POLL_MS;

// بند ۸.۱۷: وضعیت فشرده بودن دکمه انکودر در لحظه روشن شدن دستگاه
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
