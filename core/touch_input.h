#pragma once
// سرویس ورودی لمس هسته MakerCore (بند ۸.۱۷).
// فقط از hal_touch.h، touch.h و hal_time.h استفاده می‌کند. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "../config/mc_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// متغیرهای دیباگ سرویس لمس (بند ۸.۱۷)
extern volatile uint32_t mc_dbg_364_touch_pressed;     // خواندنی: ۱ = صفحه الان (بعد از debounce) فشرده است
extern volatile uint32_t mc_dbg_365_touch_raw_x;       // خواندنی: آخرین مختصات خام X
extern volatile uint32_t mc_dbg_366_touch_raw_y;       // خواندنی: آخرین مختصات خام Y
extern volatile uint32_t mc_dbg_367_touch_x;           // خواندنی: آخرین مختصات پیکسل X
extern volatile uint32_t mc_dbg_368_touch_y;           // خواندنی: آخرین مختصات پیکسل Y
extern volatile uint32_t mc_dbg_372_touch_down_cnt;    // خواندنی: تعداد رویدادهای فشردن
extern volatile uint32_t mc_dbg_373_touch_up_cnt;      // خواندنی: تعداد رویدادهای رها شدن
extern volatile uint32_t mc_dbg_376_touch_enable;      // نوشتنی: ۰ = سرویس لمس خاموش (پیش‌فرض ۱)

#ifdef __cplusplus
}

namespace mc {

// راه‌اندازی سرویس ورودی لمس (و فراخوانی mc_touch_init)
void mc_touchin_init();

// وظیفه زمان‌بند لمس (هر MC_TOUCH_POLL_MS میلی‌ثانیه)
void mc_touchin_poll();

// آیا انگشت الان (بعد از debounce) روی صفحه است؟
bool mc_touchin_is_down();

// آخرین مختصات خام معتبر وقتی انگشت روی صفحه است
bool mc_touchin_raw(uint16_t* rx, uint16_t* ry);

// مختصات پیکسل (بعد از mc_touch_apply)؛ false اگر انگشت نیست یا هنوز کالیبره نشده
bool mc_touchin_pos(uint16_t* x, uint16_t* y);

// رویداد فشردن (یک‌بار مصرف، بافر صفر می‌شود)
bool mc_touchin_take_down();

// رویداد رها شدن (یک‌بار مصرف، بافر صفر می‌شود)
bool mc_touchin_take_up();

}  // namespace mc
#endif
