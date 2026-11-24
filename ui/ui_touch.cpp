// کمک‌کار لمس برای صفحه‌های رابط کاربری (بند ۸.۱۶).
// هیچ هدر ST اینجا include نمی‌شود.
#include "ui_touch.h"
#include "../core/touch_input.h"
#include "../hal/hal_buzzer.h"
#include "../config/mc_config.h"

extern "C" {
volatile uint32_t mc_dbg_377_uitouch_tap_cnt   = 0;
volatile uint32_t mc_dbg_378_uitouch_tap_x     = 0;
volatile uint32_t mc_dbg_379_uitouch_tap_y     = 0;
volatile uint32_t mc_dbg_380_uitouch_swipe_cnt = 0;
volatile uint32_t mc_dbg_381_uitouch_beep_en   = 1;
volatile uint32_t mc_dbg_382_uitouch_beep_cnt  = 0;
volatile uint32_t mc_dbg_383_uitouch_hit_id    = 0;
}

namespace mc {
namespace {

bool     g_have_press   = false;   // یک فشردن معتبر ثبت شده و منتظر رها شدن هستیم
bool     g_wait_release = false;   // بعد از reset تا برداشتن انگشت چیزی تشخیص نده
uint16_t g_start_x      = 0U;
uint16_t g_start_y      = 0U;

int32_t abs32(int32_t v) { return (v < 0) ? -v : v; }

}  // namespace

void mc_ui_touch_reset() {
    (void)mc_touchin_take_down();
    (void)mc_touchin_take_up();
    g_have_press   = false;
    g_wait_release = mc_touchin_is_down();
}

TouchGesture mc_ui_touch_poll() {
    TouchGesture g = { TouchEvt::None, 0U, 0U };
    (void)mc_dbg_381_uitouch_beep_en;   // خواندن برای جلوگیری از حذف توسط لینکر

    if (g_wait_release) {
        (void)mc_touchin_take_down();
        (void)mc_touchin_take_up();
        if (!mc_touchin_is_down()) { g_wait_release = false; }
        return g;
    }

    if (mc_touchin_take_down()) {
        uint16_t x = 0U;
        uint16_t y = 0U;
        g_have_press = mc_touchin_press_pos(&x, &y);
        if (g_have_press) {
            g_start_x = x;
            g_start_y = y;
        }
    }

    if (mc_touchin_take_up()) {
        if (g_have_press) {
            g_have_press = false;
            uint16_t ex = g_start_x;
            uint16_t ey = g_start_y;
            (void)mc_touchin_last_pos(&ex, &ey);

            const int32_t dx  = static_cast<int32_t>(ex) - static_cast<int32_t>(g_start_x);
            const int32_t dy  = static_cast<int32_t>(ey) - static_cast<int32_t>(g_start_y);
            const int32_t adx = abs32(dx);
            const int32_t ady = abs32(dy);

            if ((adx <= static_cast<int32_t>(MC_UI_TOUCH_TAP_SLOP_PX)) &&
                (ady <= static_cast<int32_t>(MC_UI_TOUCH_TAP_SLOP_PX))) {
                g.evt = TouchEvt::Tap;
                g.x   = g_start_x;
                g.y   = g_start_y;
                mc_dbg_377_uitouch_tap_cnt = mc_dbg_377_uitouch_tap_cnt + 1U;
                mc_dbg_378_uitouch_tap_x   = g_start_x;
                mc_dbg_379_uitouch_tap_y   = g_start_y;
                mc_dbg_383_uitouch_hit_id  = 0U;   // اگر صفحه آن را به دکمه‌ای نسبت داد، mc_ui_touch_accept مقدار را عوض می‌کند
            } else if ((ady >= static_cast<int32_t>(MC_UI_TOUCH_SWIPE_MIN_PX)) && (ady > adx)) {
                g.evt = (dy < 0) ? TouchEvt::SwipeUp : TouchEvt::SwipeDown;
                g.x   = g_start_x;
                g.y   = g_start_y;
                mc_dbg_380_uitouch_swipe_cnt = mc_dbg_380_uitouch_swipe_cnt + 1U;
            }
        }
    }
    return g;
}

bool mc_ui_touch_in_rect(uint16_t px, uint16_t py, uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh) {
    const int32_t m  = static_cast<int32_t>(MC_UI_TOUCH_HIT_MARGIN_PX);
    const int32_t x  = static_cast<int32_t>(px);
    const int32_t y  = static_cast<int32_t>(py);
    const int32_t x0 = static_cast<int32_t>(rx) - m;
    const int32_t y0 = static_cast<int32_t>(ry) - m;
    const int32_t x1 = static_cast<int32_t>(rx) + static_cast<int32_t>(rw) + m;
    const int32_t y1 = static_cast<int32_t>(ry) + static_cast<int32_t>(rh) + m;
    return (x >= x0) && (x < x1) && (y >= y0) && (y < y1);
}

void mc_ui_touch_accept(uint32_t hit_id) {
    mc_dbg_383_uitouch_hit_id = hit_id;
    if (mc_dbg_381_uitouch_beep_en != 0U) {
        mc_buzzer_click(MC_UI_TOUCH_BEEP_MS);   // غیرمسدودکننده؛ خاموشی را mc_encoder_poll هر ۱ms انجام می‌دهد
        mc_dbg_382_uitouch_beep_cnt = mc_dbg_382_uitouch_beep_cnt + 1U;
    }
}

}  // namespace mc
