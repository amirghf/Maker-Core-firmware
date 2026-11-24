// خاموش و روشن شدن خودکار نمایشگر هنگام بیکاری (بند ۸.۱۹).
// هیچ هدر ST اینجا include نمی‌شود.
#include "ui_screen.h"
#include "ui_touch.h"
#include "../core/encoder.h"
#include "../core/touch_input.h"
#include "../hal/hal_display.h"
#include "../hal/hal_time.h"

extern "C" {
volatile uint32_t mc_dbg_417_scr_sleep_en   = 1;
volatile uint32_t mc_dbg_418_scr_state      = 0;
volatile uint32_t mc_dbg_419_scr_idle_ms    = 0;
volatile uint32_t mc_dbg_420_scr_timeout_s  = 0;
volatile uint32_t mc_dbg_421_scr_sleep_cnt  = 0;
volatile uint32_t mc_dbg_422_scr_wake_cnt   = 0;
volatile uint32_t mc_dbg_423_scr_wake_src   = 0;
}

namespace mc {
namespace {

bool     g_asleep           = false;   // نمایشگر الان خاموش است
uint32_t g_last_active_ms   = 0U;      // زمان آخرین فعالیت کاربر
uint32_t g_last_enc_cnt     = 0U;      // آخرین مقدار شمارنده فعالیت انکودر
uint32_t g_last_touch_cnt   = 0U;      // آخرین مقدار شمارنده فشردن لمس

// مدت بیکاری تا خاموشی (میلی‌ثانیه). 0 = خاموشی غیرفعال.
uint32_t timeout_ms() {
    const uint32_t s = mc_dbg_420_scr_timeout_s;
    if (s != 0U) {
        const uint32_t kMaxS = 86400U;   // سقف یک روز تا سرریز نشود
        return ((s > kMaxS) ? kMaxS : s) * 1000U;
    }
    return static_cast<uint32_t>(MC_UI_SCREEN_SLEEP_MS);
}

// روشن‌کردن نمایشگر. src: 0 = بدون ورودی، 1 = انکودر، 2 = لمس.
// swallow = true: ورودی بیدارکننده دور ریخته شود تا کاری اجرا نکند.
void wake_up(uint32_t src, bool swallow) {
    mc_display_backlight(true);
    g_asleep = false;
    g_last_active_ms = mc_millis();
    mc_dbg_418_scr_state = 0U;
    mc_dbg_423_scr_wake_src = src;
    mc_dbg_422_scr_wake_cnt = mc_dbg_422_scr_wake_cnt + 1U;
    if (swallow) {
        (void)mc_encoder_take_clicks();
        (void)mc_encoder_take_press();
        mc_ui_touch_reset();
    }
}

}  // namespace

void mc_ui_screen_init() {
    g_asleep = false;
    g_last_active_ms = mc_millis();
    g_last_enc_cnt   = mc_encoder_activity_count();
    g_last_touch_cnt = mc_touchin_down_count();
    mc_dbg_417_scr_sleep_en  = 1U;
    mc_dbg_418_scr_state     = 0U;
    mc_dbg_419_scr_idle_ms   = 0U;
    mc_dbg_420_scr_timeout_s = 0U;
    mc_dbg_421_scr_sleep_cnt = 0U;
    mc_dbg_422_scr_wake_cnt  = 0U;
    mc_dbg_423_scr_wake_src  = 0U;
}

void mc_ui_screen_poll(bool can_sleep) {
    (void)mc_dbg_417_scr_sleep_en;    // خواندن متغیرهای نوشتنی برای جلوگیری از حذف توسط لینکر
    (void)mc_dbg_420_scr_timeout_s;

    const uint32_t now       = mc_millis();
    const uint32_t enc_cnt   = mc_encoder_activity_count();
    const uint32_t touch_cnt = mc_touchin_down_count();

    // فعالیت: تغییر شمارنده‌ها از دور قبل، یا نگه‌داشتن دکمه انکودر یا انگشت روی صفحه
    const bool enc_act   = (enc_cnt != g_last_enc_cnt) || mc_encoder_is_pressed();
    const bool touch_act = (touch_cnt != g_last_touch_cnt) || mc_touchin_is_down();
    g_last_enc_cnt   = enc_cnt;
    g_last_touch_cnt = touch_cnt;

    const uint32_t tmo = timeout_ms();
    const bool enabled = (mc_dbg_417_scr_sleep_en != 0U) && (tmo != 0U) && can_sleep;

    if (g_asleep) {
        if (!enabled) {
            wake_up(0U, false);                         // شرط خاموشی برداشته شد؛ ورودی نخورد
        } else if (enc_act || touch_act) {
            wake_up(touch_act ? 2U : 1U, true);         // ورودی بیدارکننده اجرا نمی‌شود
        }
    } else {
        if ((!enabled) || enc_act || touch_act) {
            g_last_active_ms = now;
        } else if (mc_time_elapsed_ms(g_last_active_ms, tmo)) {
            mc_display_backlight(false);
            g_asleep = true;
            mc_dbg_418_scr_state = 1U;
            mc_dbg_421_scr_sleep_cnt = mc_dbg_421_scr_sleep_cnt + 1U;
        }
    }

    mc_dbg_419_scr_idle_ms = now - g_last_active_ms;
}

bool mc_ui_screen_is_asleep() {
    return g_asleep;
}

}  // namespace mc
