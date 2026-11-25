#pragma once
// راه اندازی و خواندن سنسور فیلامنت با ضدلرزش/debounce (بند ۸.۱۶.۴). هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "../config/mc_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// متغیرهای دیباگ سنسور فیلامنت (بند ۸.۱۶.۴ و ۹.۳)
extern volatile uint32_t mc_dbg_406_fil_raw;     // خواندنی: سطح خام پین PA4 (0 = LOW، 1 = HIGH)، بدون فیلتر
extern volatile uint32_t mc_dbg_407_fil_empty;   // خواندنی: نتیجه بعد از debounce (1 = فیلامنت تمام شده، 0 = فیلامنت هست)
extern volatile uint32_t mc_dbg_408_fil_edges;   // خواندنی: تعداد دفعاتی که سطح پایدار عوض شد (از روشن‌شدن)
extern volatile uint32_t mc_dbg_456_fil_run_en;   // نوشتنی: 1 تشخیص اتمام فعال، 0 غیرفعال (با M412 هم عوض می‌شود) (بند ۹.۳)
extern volatile uint32_t mc_dbg_457_fil_run_edges; // خواندنی: تعداد دفعات تغییر «هست» به «تمام شده» از روشن شدن (بند ۹.۳)

#ifdef __cplusplus
}

namespace mc {

// راه‌اندازی سنسور فیلامنت و تنظیم پین با MC_UI_FIL_PULL_MODE
void mc_filament_init();

// وظیفه زمان‌بند (هر ۱۰ میلی‌ثانیه): پین را می‌خواند و ضدلرزش (debounce) انجام می‌دهد
void mc_filament_poll();

// آیا فیلامنت تمام شده است؟ (برابر سطح پایدار == MC_UI_FIL_EMPTY_LEVEL)
bool mc_filament_empty();

// آیا قابلیت تشخیص اتمام فیلامنت فعال است؟ (بند ۹.۳)
bool mc_filament_runout_enabled();

// تحویل رویداد در انتظار اتمام فیلامنت (اگر بود خاموشش می‌کند و true برمی‌گرداند) (بند ۹.۳)
bool mc_filament_runout_take();

}  // namespace mc
#endif
