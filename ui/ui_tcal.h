#pragma once
// صفحه کالیبراسیون لمس (TOUCH CAL - صفحه ۹) (بند ۸.۱۷).
// فقط از هدرهای core و hal استفاده می‌کند. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// متغیرهای دیباگ صفحه کالیبراسیون لمس (بند ۸.۱۷)
extern volatile uint32_t mc_dbg_362_touch_cal_force;   // نوشتنی: ۰ بی‌اثر، ۱ شروع، ۲ لغو، ۳ پاک‌کردن
extern volatile uint32_t mc_dbg_363_touch_state;       // خواندنی: ۰ بیکار، ۱ تا ۳ انتظار نقطه، ۴ محاسبه، ۵ آزمون، ۶ ذخیره، ۷ پایان
extern volatile int      mc_dbg_369_touch_cal_ok;      // خواندنی: ۱- هنوز، ۱ موفق، ۰ ناموفق
extern volatile uint32_t mc_dbg_370_touch_cal_src;     // خواندنی: ۰ کالیبره نیست، ۱ از EEPROM، ۲ تازه کالیبره
extern volatile uint32_t mc_dbg_371_touch_save_result; // خواندنی: ۰ هنوز، ۱ موفق، ۲ ناموفق

#ifdef __cplusplus
}

namespace mc {

enum class TcalResult : uint8_t {
    Stay      = 0,
    Done      = 1,
    Cancelled = 2
};

// شروع و راه‌اندازی صفحه کالیبراسیون
void mc_ui_tcal_open();

// وظیفه زمان‌بند صفحه کالیبراسیون (غیرمسدودکننده)
TcalResult mc_ui_tcal_poll();

}  // namespace mc
#endif
