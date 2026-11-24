#pragma once
// راه اندازی و خواندن سنسور فیلامنت با ضدلرزش/debounce (بند ۸.۱۶.۴). هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "../config/mc_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// متغیرهای دیباگ سنسور فیلامنت (بند ۸.۱۶.۴)
extern volatile uint32_t mc_dbg_406_fil_raw;     // خواندنی: سطح خام پین PA4 (0 = LOW، 1 = HIGH)، بدون فیلتر
extern volatile uint32_t mc_dbg_407_fil_empty;   // خواندنی: نتیجه بعد از debounce (1 = فیلامنت تمام شده، 0 = فیلامنت هست)
extern volatile uint32_t mc_dbg_408_fil_edges;   // خواندنی: تعداد دفعاتی که سطح پایدار عوض شد (از روشن‌شدن)

#ifdef __cplusplus
}

namespace mc {

// راه‌اندازی سنسور فیلامنت و تنظیم پین با MC_UI_FIL_PULL_MODE
void mc_filament_init();

// وظیفه زمان‌بند (هر ۱۰ میلی‌ثانیه): پین را می‌خواند و ضدلرزش (debounce) انجام می‌دهد
void mc_filament_poll();

// آیا فیلامنت تمام شده است؟ (برابر سطح پایدار == MC_UI_FIL_EMPTY_LEVEL)
bool mc_filament_empty();

}  // namespace mc
#endif
