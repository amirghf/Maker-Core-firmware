#pragma once
// خاموش و روشن شدن خودکار نمایشگر هنگام بیکاری (بند ۸.۱۹).
// فقط از هدرهای core و hal استفاده می‌کند. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "../config/mc_config.h"

#ifdef __cplusplus
extern "C" {
#endif

extern volatile uint32_t mc_dbg_417_scr_sleep_en;     // نوشتنی: 0 = نمایشگر هیچ‌وقت خاموش نشود (پیش‌فرض 1)
extern volatile uint32_t mc_dbg_418_scr_state;        // خواندنی: 0 = نمایشگر روشن، 1 = خاموش (بک‌لایت خاموش)
extern volatile uint32_t mc_dbg_419_scr_idle_ms;      // خواندنی: مدت بیکاری از آخرین چرخش یا فشردن انکودر یا لمس (میلی‌ثانیه)
extern volatile uint32_t mc_dbg_420_scr_timeout_s;    // نوشتنی: مدت بیکاری تا خاموشی به ثانیه. 0 = از ماکرو MC_UI_SCREEN_SLEEP_MS (پیش‌فرض 0)
extern volatile uint32_t mc_dbg_421_scr_sleep_cnt;    // خواندنی: تعداد دفعاتی که نمایشگر خاموش شد
extern volatile uint32_t mc_dbg_422_scr_wake_cnt;     // خواندنی: تعداد دفعاتی که نمایشگر روشن شد
extern volatile uint32_t mc_dbg_423_scr_wake_src;     // خواندنی: منبع آخرین روشن‌شدن (0 = بدون ورودی یعنی شرط خاموشی برداشته شد، 1 = انکودر، 2 = لمس)

#ifdef __cplusplus
}

namespace mc {

// مقدار اولیه متغیرها و شمارنده‌ها (از انتهای mc_ui_init صدا زده می‌شود).
void mc_ui_screen_init();

// در هر دور mc_ui_poll یک بار صدا زده شود (قبل از پردازش صفحه‌ها).
// can_sleep = true یعنی الان مجاز است نمایشگر خاموش شود.
void mc_ui_screen_poll(bool can_sleep);

// آیا نمایشگر الان خاموش است؟
bool mc_ui_screen_is_asleep();

}  // namespace mc
#endif
