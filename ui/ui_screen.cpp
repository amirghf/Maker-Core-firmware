// خاموش و روشن شدن خودکار نمایشگر هنگام بیکاری (بند ۸.۱۹).
// هیچ هدر ST اینجا include نمی‌شود.
#include "ui_screen.h"
#include "ui_touch.h"
#include "../core/encoder.h"
#include "../core/touch_input.h"
#include "../core/disp_settings.h"
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
volatile uint32_t mc_dbg_428_scr_menu_tmo_s = 0;
volatile uint32_t mc_dbg_429_scr_exit_cnt   = 0;
}

namespace mc {
namespace {

bool     g_asleep           = false;   // نمایشگر الان خاموش است
uint32_t g_last_active_ms   = 0U;      // زمان آخرین فعالیت کاربر
uint32_t g_last_enc_cnt     = 0U;      // آخرین مقدار شمارنده فعالیت انکودر
uint32_t g_last_touch_cnt   = 0U;      // آخرین مقدار شمارنده فشردن لمس

// تبدیل ثانیه به میلی‌ثانیه با سقف یک روز (تا سرریز نشود)
uint32_t seconds_to_ms(uint32_t s) {
    const uint32_t kMaxS = 86400U;
    return ((s > kMaxS) ? kMaxS : s) * 1000U;
}

// مدت بیکاری تا خاموشی (میلی‌ثانیه). 0 = خاموشی غیرفعال.
// اولویت: متغیر debug 420 (اگر غیرصفر) و بعد تنظیم ذخیره‌شده صفحه DISPLAY.
uint32_t timeout_ms() {
    const uint32_t s = mc_dbg_420_scr_timeout_s;
    if (s != 0U) {
        return seconds_to_ms(s);
    }
    return seconds_to_ms(mc_dispset_screen_off_s());
}

// مدت بیکاری تا خروج خودکار از منو (میلی‌ثانیه). 0 = هیچ‌وقت.
// اولویت: متغیر debug 428 (اگر غیرصفر) و بعد تنظیم ذخیره‌شده صفحه DISPLAY.
uint32_t menu_exit_ms() {
    const uint32_t s = mc_dbg_428_scr_menu_tmo_s;
    if (s != 0U) {
        return seconds_to_ms(s);
    }
    return seconds_to_ms(mc_dispset_menu_exit_s());
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
    mc_dbg_428_scr_menu_tmo_s = 0U;
    mc_dbg_429_scr_exit_cnt = 0U;
}

bool mc_ui_screen_poll(bool can_sleep, bool can_exit) {
    (void)mc_dbg_417_scr_sleep_en;    // خواندن متغیرهای نوشتنی برای جلوگیری از حذف توسط لینکر
    (void)mc_dbg_420_scr_timeout_s;
    (void)mc_dbg_428_scr_menu_tmo_s;
    (void)mc_dbg_429_scr_exit_cnt;

    const uint32_t now       = mc_millis();
    const uint32_t enc_cnt   = mc_encoder_activity_count();
    const uint32_t touch_cnt = mc_touchin_down_count();

    // فعالیت: تغییر شمارنده‌ها از دور قبل، یا نگه‌داشتن دکمه انکودر یا انگشت روی صفحه
    const bool enc_act   = (enc_cnt != g_last_enc_cnt) || mc_encoder_is_pressed();
    const bool touch_act = (touch_cnt != g_last_touch_cnt) || mc_touchin_is_down();
    g_last_enc_cnt   = enc_cnt;
    g_last_touch_cnt = touch_cnt;

    const uint32_t tmo   = timeout_ms();
    const uint32_t mtmo  = menu_exit_ms();
    const bool enabled   = (mc_dbg_417_scr_sleep_en != 0U) && (tmo != 0U) && can_sleep;
    const bool exit_armed = can_exit && (mtmo != 0U);
    bool exit_due = false;

    if (g_asleep) {
        if (!enabled) {
            wake_up(0U, false);                         // شرط خاموشی برداشته شد؛ ورودی نخورد
        } else if (enc_act || touch_act) {
            wake_up(touch_act ? 2U : 1U, true);         // ورودی بیدارکننده اجرا نمی‌شود
        }
    } else {
        if (enc_act || touch_act) {
            g_last_active_ms = now;
        } else if (exit_armed && mc_time_elapsed_ms(g_last_active_ms, mtmo)) {
            // بند ۸.۲۰: بیکاری در منو یا صفحه داخلی؛ برگشت به صفحه اصلی و شروع دوباره تایمر برای خاموشی
            g_last_active_ms = now;
            mc_dbg_429_scr_exit_cnt = mc_dbg_429_scr_exit_cnt + 1U;
            exit_due = true;
        } else if (enabled && mc_time_elapsed_ms(g_last_active_ms, tmo)) {
            mc_display_backlight(false);
            g_asleep = true;
            mc_dbg_418_scr_state = 1U;
            mc_dbg_421_scr_sleep_cnt = mc_dbg_421_scr_sleep_cnt + 1U;
        } else if ((!enabled) && (!exit_armed)) {
            g_last_active_ms = now;                     // نه خاموشی فعال است نه خروج؛ تایمر جلو نرود
        }
    }

    mc_dbg_419_scr_idle_ms = now - g_last_active_ms;
    return exit_due;
}

bool mc_ui_screen_is_asleep() {
    return g_asleep;
}

void mc_ui_screen_wake() {
    if (g_asleep) {
        wake_up(0U, false);
    } else {
        mc_display_backlight(true);
        g_last_active_ms = mc_millis();
    }
}

}  // namespace mc
