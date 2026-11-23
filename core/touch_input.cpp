// سرویس ورودی لمس هسته MakerCore (بند ۸.۱۷).
// فقط از hal_touch.h، touch.h و hal_time.h استفاده می‌کند. هیچ هدر ST اینجا include نمی‌شود.
#include "touch_input.h"
#include "touch.h"
#include "hal_touch.h"
#include "hal_time.h"

extern "C" {
volatile uint32_t mc_dbg_364_touch_pressed  = 0;
volatile uint32_t mc_dbg_365_touch_raw_x    = 0;
volatile uint32_t mc_dbg_366_touch_raw_y    = 0;
volatile uint32_t mc_dbg_367_touch_x        = 0;
volatile uint32_t mc_dbg_368_touch_y        = 0;
volatile uint32_t mc_dbg_372_touch_down_cnt = 0;
volatile uint32_t mc_dbg_373_touch_up_cnt   = 0;
volatile uint32_t mc_dbg_376_touch_enable   = 1;
}

namespace mc {
namespace {

bool     g_inited            = false;
bool     g_is_down           = false;
bool     g_pending_down      = false;
bool     g_pending_up        = false;
uint8_t  g_down_confirm_cnt  = 0U;
uint8_t  g_up_confirm_cnt    = 0U;

uint16_t g_last_raw_x        = 0U;
uint16_t g_last_raw_y        = 0U;
uint16_t g_smooth_raw_x      = 0U;
uint16_t g_smooth_raw_y      = 0U;
bool     g_has_prev_raw      = false;

uint16_t g_last_pos_x        = 0U;
uint16_t g_last_pos_y        = 0U;
bool     g_pos_valid         = false;

uint32_t g_down_cnt          = 0U;
uint32_t g_up_cnt            = 0U;

}  // namespace

void mc_touchin_init() {
    (void)mc_touch_init();
    g_inited           = true;
    g_is_down          = false;
    g_pending_down     = false;
    g_pending_up       = false;
    g_down_confirm_cnt = 0U;
    g_up_confirm_cnt   = 0U;
    g_has_prev_raw     = false;
    g_pos_valid        = false;
    g_down_cnt         = 0U;
    g_up_cnt           = 0U;

    mc_dbg_364_touch_pressed  = 0U;
    mc_dbg_365_touch_raw_x    = 0U;
    mc_dbg_366_touch_raw_y    = 0U;
    mc_dbg_367_touch_x        = 0U;
    mc_dbg_368_touch_y        = 0U;
    mc_dbg_372_touch_down_cnt = 0U;
    mc_dbg_373_touch_up_cnt   = 0U;
}

void mc_touchin_poll() {
    if (!g_inited) { return; }

    uint16_t rx = 0U;
    uint16_t ry = 0U;
    bool pressed = false;

    // کلید ایمن: اگر 0 باشد، هیچ SPI خوانده نمی‌شود و لمس رها حساب می‌شود
    if (mc_dbg_376_touch_enable != 0U) {
        pressed = mc_touch_read_raw(&rx, &ry);
    }

    if (pressed) {
        g_up_confirm_cnt = 0U;
        g_down_confirm_cnt = static_cast<uint8_t>(g_down_confirm_cnt + 1U);

        // فیلتر ساده عدد صحیح: میانگین دو خواندن خام آخر
        if (g_has_prev_raw) {
            g_smooth_raw_x = static_cast<uint16_t>((rx + g_last_raw_x) / 2U);
            g_smooth_raw_y = static_cast<uint16_t>((ry + g_last_raw_y) / 2U);
        } else {
            g_smooth_raw_x = rx;
            g_smooth_raw_y = ry;
            g_has_prev_raw = true;
        }
        g_last_raw_x = rx;
        g_last_raw_y = ry;

        if (g_down_confirm_cnt >= MC_TOUCH_DOWN_CONFIRM) {
            if (!g_is_down) {
                g_is_down = true;
                g_pending_down = true;
                g_down_cnt = g_down_cnt + 1U;
                mc_dbg_372_touch_down_cnt = g_down_cnt;
            }

            // تبدیل مختصات خام نرم‌شده به پیکسل
            uint16_t px = 0U;
            uint16_t py = 0U;
            if (mc_touch_apply(g_smooth_raw_x, g_smooth_raw_y, &px, &py)) {
                g_last_pos_x = px;
                g_last_pos_y = py;
                g_pos_valid  = true;
            } else {
                g_pos_valid  = false;
            }
        }
    } else {
        g_down_confirm_cnt = 0U;
        g_has_prev_raw = false;
        g_up_confirm_cnt = static_cast<uint8_t>(g_up_confirm_cnt + 1U);

        if (g_up_confirm_cnt >= MC_TOUCH_UP_CONFIRM) {
            if (g_is_down) {
                g_is_down = false;
                g_pending_up = true;
                g_pos_valid = false;
                g_up_cnt = g_up_cnt + 1U;
                mc_dbg_373_touch_up_cnt = g_up_cnt;
            }
        }
    }

    // به‌روزرسانی متغیرهای دیباگ
    mc_dbg_364_touch_pressed = g_is_down ? 1U : 0U;
    if (g_is_down) {
        mc_dbg_365_touch_raw_x = g_smooth_raw_x;
        mc_dbg_366_touch_raw_y = g_smooth_raw_y;
        if (g_pos_valid) {
            mc_dbg_367_touch_x = g_last_pos_x;
            mc_dbg_368_touch_y = g_last_pos_y;
        }
    }
}

bool mc_touchin_is_down() {
    return g_is_down;
}

bool mc_touchin_raw(uint16_t* rx, uint16_t* ry) {
    if ((!g_is_down) || (rx == nullptr) || (ry == nullptr)) { return false; }
    *rx = g_smooth_raw_x;
    *ry = g_smooth_raw_y;
    return true;
}

bool mc_touchin_pos(uint16_t* x, uint16_t* y) {
    if ((!g_is_down) || (!g_pos_valid) || (x == nullptr) || (y == nullptr)) { return false; }
    *x = g_last_pos_x;
    *y = g_last_pos_y;
    return true;
}

bool mc_touchin_take_down() {
    if (g_pending_down) {
        g_pending_down = false;
        return true;
    }
    return false;
}

bool mc_touchin_take_up() {
    if (g_pending_up) {
        g_pending_up = false;
        return true;
    }
    return false;
}

}  // namespace mc
