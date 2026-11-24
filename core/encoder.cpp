// ماژول انکودر چرخشی و دکمه (بند ۸.۸).
// فقط از هدرهای hal استفاده می‌کند. هیچ هدر ST اینجا include نمی‌شود.
#include "encoder.h"
#include "../hal/hal_gpio.h"
#include "../hal/hal_buzzer.h"
#include "../hal/hal_time.h"
#include "../config/mc_config.h"

extern "C" {
volatile int32_t  mc_dbg_313_enc_pos       = 0;  // خواندنی: جمع کل کلیک‌های چرخش از روشن‌شدن (با علامت)
volatile uint32_t mc_dbg_314_enc_btn       = 0;  // خواندنی: ۱ = دکمه انکودر الان فشرده است
volatile uint32_t mc_dbg_315_enc_press_cnt = 0;  // خواندنی: تعداد فشردن‌های دکمه
volatile uint32_t mc_dbg_316_enc_beep_cnt  = 0;  // خواندنی: تعداد بوق‌های درخواست‌شده توسط انکودر
volatile uint32_t mc_dbg_317_enc_beep_en   = 1;  // نوشتنی: ۰ = بوق انکودر خاموش (پیش‌فرض ۱)
}

namespace mc {
namespace {

// وضعیت قبلی ۲ سیگنال انکودر (۲ بیت: A بالا، B پایین)
static uint8_t g_prev_ab = 0U;

// انباشت‌گر گام‌های کوادراتور برای محاسبه کلیک
static int8_t  g_step_acc = 0;

// کلیک‌های چرخش جمع‌شده برای مصرف‌کننده (با take_clicks صفر می‌شود)
static int32_t g_pending_clicks = 0;

// پرچم فشردن دکمه برای مصرف‌کننده (با take_press صفر می‌شود)
static bool g_pressed_flag = false;

// شمارنده فعالیت انکودر (مصرف نمی‌شود) (بند ۸.۱۹)
static uint32_t g_activity_cnt = 0U;

// وضعیت دکمه برای ضدلرزش
static bool     g_btn_raw_state       = false;
static bool     g_btn_debounced_state = false;
static uint32_t g_btn_last_change_ms  = 0U;

// جدول تغییر حالت کوادراتور (اندیس = (prev_ab << 2) | curr_ab)
// 0 = بدون تغییر یا پرش دوتایی نامعتبر، +1 = چرخش ساعتگرد، -1 = چرخش پادساعتگرد
static const int8_t kQuadTable[16] = {
     0,  1, -1,  0,
    -1,  0,  0,  1,
     1,  0,  0, -1,
     0, -1,  1,  0
};

}  // namespace

void mc_encoder_init() {
    // تنظیم حالت پین‌های انکودر (با پول‌آپ)
    mc_gpio_set_mode(PinId::EncA, PinMode::InputPullUp);
    mc_gpio_set_mode(PinId::EncB, PinMode::InputPullUp);
    mc_gpio_set_mode(PinId::EncBtn, PinMode::InputPullUp);

    const bool a = mc_gpio_read(PinId::EncA);
    const bool b = mc_gpio_read(PinId::EncB);
    g_prev_ab = static_cast<uint8_t>((a ? 2U : 0U) | (b ? 1U : 0U));

    const bool btn_pressed = !mc_gpio_read(PinId::EncBtn);
    g_btn_raw_state = btn_pressed;
    g_btn_debounced_state = btn_pressed;
    g_btn_last_change_ms = mc_millis();

    g_step_acc = 0;
    g_pending_clicks = 0;
    g_pressed_flag = false;
    g_activity_cnt = 0U;

    mc_dbg_313_enc_pos = 0;
    mc_dbg_314_enc_btn = btn_pressed ? 1U : 0U;
    mc_dbg_315_enc_press_cnt = 0U;
    mc_dbg_316_enc_beep_cnt = 0U;
    mc_dbg_317_enc_beep_en = 1U;
}

void mc_encoder_poll() {
    (void)mc_dbg_317_enc_beep_en;  // خواندن متغیر نوشتنی جهت جلوگیری از حذف لینکر

    // ۱. خواندن سیگنال‌های چرخشی A و B
    const bool a = mc_gpio_read(PinId::EncA);
    const bool b = mc_gpio_read(PinId::EncB);
    const uint8_t curr_ab = static_cast<uint8_t>((a ? 2U : 0U) | (b ? 1U : 0U));

    if (curr_ab != g_prev_ab) {
        const uint8_t index = static_cast<uint8_t>((g_prev_ab << 2U) | curr_ab);
        const int8_t step = kQuadTable[index & 0x0FU];
        g_prev_ab = curr_ab;

        if (step != 0) {
            g_step_acc = static_cast<int8_t>(g_step_acc + step);

            const int8_t steps_per_click = static_cast<int8_t>(MC_ENC_STEPS_PER_CLICK);
            if (g_step_acc >= steps_per_click) {
                g_step_acc = static_cast<int8_t>(g_step_acc - steps_per_click);
                const int32_t dir = (MC_ENC_REVERSE != 0) ? -1 : 1;
                g_pending_clicks += dir;
                g_activity_cnt = g_activity_cnt + 1U;
                mc_dbg_313_enc_pos += dir;
            } else if (g_step_acc <= -steps_per_click) {
                g_step_acc = static_cast<int8_t>(g_step_acc + steps_per_click);
                const int32_t dir = (MC_ENC_REVERSE != 0) ? 1 : -1;
                g_pending_clicks += dir;
                g_activity_cnt = g_activity_cnt + 1U;
                mc_dbg_313_enc_pos += dir;
            }
        }
    }

    // ۲. خواندن دکمه انکودر با ضدلرزش (فشردن = LOW)
    const bool raw_pressed = !mc_gpio_read(PinId::EncBtn);
    const uint32_t now_ms = mc_millis();

    if (raw_pressed != g_btn_raw_state) {
        g_btn_raw_state = raw_pressed;
        g_btn_last_change_ms = now_ms;
    }

    if (mc_time_elapsed_ms(g_btn_last_change_ms, MC_ENC_BTN_DEBOUNCE_MS)) {
        if (g_btn_debounced_state != g_btn_raw_state) {
            g_btn_debounced_state = g_btn_raw_state;
            if (g_btn_debounced_state) {
                // لبه بالارونده فشردن دکمه
                g_pressed_flag = true;
                g_activity_cnt = g_activity_cnt + 1U;
                mc_dbg_315_enc_press_cnt = mc_dbg_315_enc_press_cnt + 1U;

                if (mc_dbg_317_enc_beep_en != 0U) {
                    mc_buzzer_click(MC_ENC_PRESS_BEEP_MS);
                    mc_dbg_316_enc_beep_cnt = mc_dbg_316_enc_beep_cnt + 1U;
                }
            }
        }
    }

    mc_dbg_314_enc_btn = g_btn_debounced_state ? 1U : 0U;

    // ۳. مدیریت زمان خاموشی غیرمسدودکننده بیزر
    mc_buzzer_click_poll();
}

int32_t mc_encoder_take_clicks() {
    const int32_t val = g_pending_clicks;
    g_pending_clicks = 0;
    return val;
}

bool mc_encoder_take_press() {
    const bool val = g_pressed_flag;
    g_pressed_flag = false;
    return val;
}

bool mc_encoder_is_pressed() {
    return g_btn_debounced_state;
}

uint32_t mc_encoder_activity_count() {
    return g_activity_cnt;
}

}  // namespace mc
