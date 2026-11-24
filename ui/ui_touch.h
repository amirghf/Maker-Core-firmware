#pragma once
// کمک‌کار لمس برای صفحه‌های رابط کاربری (بند ۸.۱۶): تشخیص ضربه، کشیدن عمودی و بوق لمس.
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern volatile uint32_t mc_dbg_377_uitouch_tap_cnt;    // خواندنی: تعداد ضربه‌های کامل تشخیص‌داده‌شده
extern volatile uint32_t mc_dbg_378_uitouch_tap_x;      // خواندنی: X آخرین ضربه (پیکسل)
extern volatile uint32_t mc_dbg_379_uitouch_tap_y;      // خواندنی: Y آخرین ضربه (پیکسل)
extern volatile uint32_t mc_dbg_380_uitouch_swipe_cnt;  // خواندنی: تعداد کشیدن‌های عمودی تشخیص‌داده‌شده
extern volatile uint32_t mc_dbg_381_uitouch_beep_en;    // نوشتنی: 0 = بوق لمس خاموش (پیش‌فرض 1)
extern volatile uint32_t mc_dbg_382_uitouch_beep_cnt;   // خواندنی: تعداد بوق‌های لمس
extern volatile uint32_t mc_dbg_383_uitouch_hit_id;     // خواندنی: هدف آخرین ضربه. 0 = جای خالی، 1..5 = دکمه‌های صفحه اصلی (START, PAUSE, STOP, RESET, MENU)، 10..17 = آیتم منو با اندیس 0..7 (10 + اندیس)، 20 = دکمه BACK صفحه‌های داخلی، 30 تا 32 = دکمه‌های HEAT اول، HEAT دوم و COOL صفحه داغ، 40 تا 43 = دکمه‌های HEAT و COOL و FEED منفی و FEED مثبت صفحه اکسترودر، 44 تا 46 = سه چیپ گام FEED

#ifdef __cplusplus
}

namespace mc {

enum class TouchEvt : uint8_t { None = 0, Tap = 1, SwipeUp = 2, SwipeDown = 3 };

struct TouchGesture {
    TouchEvt evt;   // نوع رویداد
    uint16_t x;     // X نقطه شروع (پیکسل)
    uint16_t y;     // Y نقطه شروع (پیکسل)
};

// رویدادهای قدیمی لمس را دور می‌ریزد و تا برداشتن انگشت هیچ ضربه‌ای تشخیص نمی‌دهد.
// هنگام باز شدن هر صفحه یا برگشت به صفحه اصلی صدا زده شود.
void mc_ui_touch_reset();

// در هر دور poll صفحه (فقط در حالت کار عادی، بعد از پایان نقاشی اولیه) یک بار صدا زده شود.
// ضربه یا کشیدن عمودی کامل‌شده را برمی‌گرداند (در هر دور حداکثر یک رویداد).
TouchGesture mc_ui_touch_poll();

// آیا نقطه (px, py) داخل مستطیل (rx, ry, rw, rh) با حاشیه MC_UI_TOUCH_HIT_MARGIN_PX است؟
bool mc_ui_touch_in_rect(uint16_t px, uint16_t py, uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh);

// وقتی ضربه به یک دکمه خورد صدا زده شود: hit_id را ثبت می‌کند و (اگر بوق روشن است) یک بوق کوتاه می‌زند.
void mc_ui_touch_accept(uint32_t hit_id);

}  // namespace mc
#endif
