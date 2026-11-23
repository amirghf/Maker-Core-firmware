// صفحه کالیبراسیون لمس (TOUCH CAL - صفحه ۹) (بند ۸.۱۷).
// هیچ هدر ST اینجا include نمی‌شود. فقط از hal و gfx و encoder استفاده می‌کند.
#include "ui_tcal.h"
#include "ui_colors.h"
#include "../core/gfx.h"
#include "../core/touch.h"
#include "../core/touch_input.h"
#include "../core/touch_cal_store.h"
#include "../core/encoder.h"
#include "../core/text_format.h"
#include "../hal/hal_display.h"
#include "../hal/hal_time.h"
#include <stdlib.h>
#include <string.h>


extern "C" {
volatile uint32_t mc_dbg_362_touch_cal_force   = 0;
volatile uint32_t mc_dbg_363_touch_state       = 0;
volatile int      mc_dbg_369_touch_cal_ok      = -1;
volatile uint32_t mc_dbg_370_touch_cal_src     = 0;
volatile uint32_t mc_dbg_371_touch_save_result = 0;
}

namespace mc {
namespace {

constexpr uint16_t kCalX[3] = { 40U, 440U, 240U };
constexpr uint16_t kCalY[3] = { 40U, 160U, 280U };

enum class Step : uint8_t {
    Init           = 0,
    WaitTouch      = 1,
    WaitRelease    = 2,
    Calculate      = 3,
    TestScreen     = 4,
    SaveStore      = 5,
    ResultMessage  = 6
};

Step     g_step        = Step::Init;
uint8_t  g_point_idx   = 0U;
uint8_t  g_retry_cnt   = 0U;
uint16_t g_raw_x[3]    = { 0U, 0U, 0U };
uint16_t g_raw_y[3]    = { 0U, 0U, 0U };
uint32_t g_t_timer     = 0U;
bool     g_save_ok     = false;

int16_t abs_diff(uint16_t a, uint16_t b) {
    return (a >= b) ? static_cast<int16_t>(a - b) : static_cast<int16_t>(b - a);
}

// رسم علامت + به اندازه ۲۱ در ۲۱ پیکسل
void draw_cross(uint16_t cx, uint16_t cy, uint16_t color) {
    mc_gfx_hline(static_cast<uint16_t>(cx - 10U), cy, 21U, color);
    mc_gfx_vline(cx, static_cast<uint16_t>(cy - 10U), 21U, color);
}

// رسم نوار بالای صفحه کالیبراسیون
void draw_top_bar() {
    mc_gfx_fill_rect(0U, 0U, 480U, 36U, kTopBarBg);
    mc_gfx_fill_rect(0U, 36U, 480U, 2U, kAccentLine);
    const uint16_t tw = mc_gfx_text_width("TOUCH CALIBRATION", 2U);
    const uint16_t tx = static_cast<uint16_t>((480U - tw) / 2U);
    mc_gfx_text(tx, 10U, "TOUCH CALIBRATION", kColorWhite, kTopBarBg, 2U);
}

// رسم وضعیت یک نقطه از سه نقطه
void draw_point_screen(uint8_t pt_idx) {
    mc_display_fill_screen(kScreenBg);
    draw_top_bar();

    char msg[32];
    msg[0] = '\0';
    mc_fmt_text(msg, sizeof(msg), 0U, "Touch the cross  ");
    const uint16_t len = static_cast<uint16_t>(strlen(msg));
    msg[len] = static_cast<char>('1' + pt_idx);
    msg[len + 1U] = '\0';
    mc_fmt_text(msg, sizeof(msg), len + 1U, " of 3");

    const uint16_t tw = mc_gfx_text_width(msg, 2U);
    const uint16_t tx = static_cast<uint16_t>((480U - tw) / 2U);
    mc_gfx_text(tx, 60U, msg, kColorWhite, kScreenBg, 2U);

    // راهنمای انکودر در پایین
    const uint16_t gw = mc_gfx_text_width("Press encoder to cancel", 2U);
    const uint16_t gx = static_cast<uint16_t>((480U - gw) / 2U);
    mc_gfx_text(gx, 280U, "Press encoder to cancel", kLabelGray, kScreenBg, 2U);

    // رسم سه نقطه
    for (uint8_t i = 0U; i < 3U; ++i) {
        if (i < pt_idx) {
            draw_cross(kCalX[i], kCalY[i], kColorGreen);
        } else if (i == pt_idx) {
            draw_cross(kCalX[i], kCalY[i], kColorRed);
        } else {
            draw_cross(kCalX[i], kCalY[i], kCardBorder);
        }
    }
}

}  // namespace

void mc_ui_tcal_open() {
    g_step      = Step::Init;
    g_point_idx = 0U;
    g_retry_cnt = 0U;
    g_save_ok   = false;

    mc_dbg_363_touch_state   = 0U;
    mc_dbg_369_touch_cal_ok  = -1;

    (void)mc_encoder_take_clicks();
    (void)mc_encoder_take_press();
}

TcalResult mc_ui_tcal_poll() {
    // چرخش انکودر اثری ندارد
    (void)mc_encoder_take_clicks();

    // دکمه انکودر در گام‌های ۰ تا ۳ کالیبراسیون را لغو می‌کند
    if ((g_step <= Step::Calculate) && mc_encoder_take_press()) {
        mc_dbg_363_touch_state = 0U;
        return TcalResult::Cancelled;
    }

    switch (g_step) {
        case Step::Init: {
            g_point_idx = 0U;
            draw_point_screen(g_point_idx);
            g_step = Step::WaitTouch;
            mc_dbg_363_touch_state = 1U;
            return TcalResult::Stay;
        }

        case Step::WaitTouch: {
            if (mc_touchin_take_down()) {
                uint16_t rx = 0U;
                uint16_t ry = 0U;
                if (mc_touchin_raw(&rx, &ry)) {
                    // اعتبارسنجی فاصله خام از نقطه‌های قبلی
                    bool spread_ok = true;
                    for (uint8_t i = 0U; i < g_point_idx; ++i) {
                        if ((abs_diff(rx, g_raw_x[i]) < MC_TOUCH_CAL_MIN_SPREAD) &&
                            (abs_diff(ry, g_raw_y[i]) < MC_TOUCH_CAL_MIN_SPREAD)) {
                            spread_ok = false;
                            break;
                        }
                    }

                    if (!spread_ok) {
                        const uint16_t tw = mc_gfx_text_width("Touch a different spot", 2U);
                        const uint16_t tx = static_cast<uint16_t>((480U - tw) / 2U);
                        mc_gfx_text(tx, 140U, "Touch a different spot", kColorRed, kScreenBg, 2U);
                        g_t_timer = mc_millis();
                        // منتظر مکث می‌مانیم و دوباره همان نقطه را می‌خواهیم
                        g_step = Step::WaitRelease;
                        return TcalResult::Stay;
                    }

                    g_raw_x[g_point_idx] = rx;
                    g_raw_y[g_point_idx] = ry;

                    // سبز کردن علامت نقطه گرفته‌شده
                    draw_cross(kCalX[g_point_idx], kCalY[g_point_idx], kColorGreen);

                    g_t_timer = mc_millis();
                    g_step = Step::WaitRelease;
                }
            }
            return TcalResult::Stay;
        }

        case Step::WaitRelease: {
            (void)mc_touchin_take_up();
            if (!mc_touchin_is_down() && mc_time_elapsed_ms(g_t_timer, MC_TOUCH_CAL_RELEASE_MS)) {
                // اگر نقطه رد شده بود، دوباره همان نقطه را نشان بده
                if (g_raw_x[g_point_idx] == 0U && g_raw_y[g_point_idx] == 0U) {
                    draw_point_screen(g_point_idx);
                    g_step = Step::WaitTouch;
                    return TcalResult::Stay;
                }

                g_point_idx = static_cast<uint8_t>(g_point_idx + 1U);
                if (g_point_idx < 3U) {
                    draw_point_screen(g_point_idx);
                    mc_dbg_363_touch_state = static_cast<uint32_t>(g_point_idx + 1U);
                    g_step = Step::WaitTouch;
                } else {
                    g_step = Step::Calculate;
                }
            }
            return TcalResult::Stay;
        }

        case Step::Calculate: {
            mc_dbg_363_touch_state = 4U;
            const bool ok = mc_touch_calibrate_3pt(g_raw_x, g_raw_y, kCalX, kCalY);
            mc_dbg_369_touch_cal_ok = ok ? 1 : 0;

            if (!ok) {
                mc_display_fill_screen(kScreenBg);
                draw_top_bar();
                const uint16_t tw = mc_gfx_text_width("CALIBRATION FAILED", 2U);
                const uint16_t tx = static_cast<uint16_t>((480U - tw) / 2U);
                mc_gfx_text(tx, 140U, "CALIBRATION FAILED", kColorRed, kScreenBg, 2U);

                g_retry_cnt = static_cast<uint8_t>(g_retry_cnt + 1U);
                if (g_retry_cnt < MC_TOUCH_CAL_MAX_RETRY) {
                    // بعد از ۱.۵ ثانیه دوباره از نقطه ۱ شروع کن
                    g_t_timer = mc_millis();
                    while (!mc_time_elapsed_ms(g_t_timer, 1500U)) { /* short pause */ }
                    g_point_idx = 0U;
                    g_raw_x[0] = g_raw_x[1] = g_raw_x[2] = 0U;
                    g_raw_y[0] = g_raw_y[1] = g_raw_y[2] = 0U;
                    g_step = Step::Init;
                    return TcalResult::Stay;
                } else {
                    return TcalResult::Cancelled;
                }
            }

            // محاسبه موفق شد؛ به صفحه آزمون برو
            mc_display_fill_screen(kScreenBg);
            const uint16_t tw = mc_gfx_text_width("Test: draw, encoder = save", 2U);
            const uint16_t tx = static_cast<uint16_t>((480U - tw) / 2U);
            mc_gfx_text(tx, 10U, "Test: draw, encoder = save", kColorWhite, kScreenBg, 2U);

            // پنج علامت + سفید در چهار گوشه و مرکز
            draw_cross(20U, 20U, kColorWhite);
            draw_cross(460U, 20U, kColorWhite);
            draw_cross(20U, 300U, kColorWhite);
            draw_cross(460U, 300U, kColorWhite);
            draw_cross(240U, 160U, kColorWhite);

            g_t_timer = mc_millis();
            g_step = Step::TestScreen;
            mc_dbg_363_touch_state = 5U;
            return TcalResult::Stay;
        }

        case Step::TestScreen: {
            mc_dbg_363_touch_state = 5U;

            if (mc_touchin_is_down()) {
                uint16_t px = 0U;
                uint16_t py = 0U;
                if (mc_touchin_pos(&px, &py)) {
                    const uint16_t dx = (px >= 2U) ? static_cast<uint16_t>(px - 2U) : 0U;
                    const uint16_t dy = (py >= 2U) ? static_cast<uint16_t>(py - 2U) : 0U;
                    mc_display_fill_rect(dx, dy, 5U, 5U, kColorWhite);

                    // نمایش مختصات زنده پیکسل
                    char pos_str[24];
                    pos_str[0] = '\0';
                    mc_fmt_text(pos_str, sizeof(pos_str), 0U, "X: ");
                    uint16_t l = static_cast<uint16_t>(strlen(pos_str));
                    l = mc_fmt_uint(pos_str, sizeof(pos_str), l, px);
                    mc_fmt_text(pos_str, sizeof(pos_str), l, "  Y: ");
                    l = static_cast<uint16_t>(strlen(pos_str));
                    (void)mc_fmt_uint(pos_str, sizeof(pos_str), l, py);

                    mc_gfx_text(180U, 260U, pos_str, kAccFan, kScreenBg, 2U);
                }
            }

            // فشردن انکودر یا پایان زمان (۲۰ ثانیه) آزمون را تمام می‌کند
            if (mc_encoder_take_press() || mc_time_elapsed_ms(g_t_timer, MC_TOUCH_TEST_MS)) {
                (void)mc_tcal_store_save_start();
                g_step = Step::SaveStore;
                mc_dbg_363_touch_state = 6U;
            }
            return TcalResult::Stay;
        }

        case Step::SaveStore: {
            mc_dbg_363_touch_state = 6U;
            mc_tcal_store_poll();

            const uint8_t st = mc_tcal_store_state();
            if (st == 1U) { return TcalResult::Stay; }   // هنوز در جریان است

            const uint8_t res = mc_tcal_store_result();
            g_save_ok = (res == 1U);

            if (g_save_ok) {
                mc_dbg_371_touch_save_result = 1U;
                mc_dbg_370_touch_cal_src = 2U;   // تازه کالیبره و ذخیره شد
            } else {
                mc_dbg_371_touch_save_result = 2U;
            }

            mc_display_fill_screen(kScreenBg);
            if (g_save_ok) {
                const uint16_t tw = mc_gfx_text_width("SAVED", 3U);
                const uint16_t tx = static_cast<uint16_t>((480U - tw) / 2U);
                mc_gfx_text(tx, 130U, "SAVED", kColorGreen, kScreenBg, 3U);
            } else {
                const uint16_t tw = mc_gfx_text_width("SAVE FAILED", 3U);
                const uint16_t tx = static_cast<uint16_t>((480U - tw) / 2U);
                mc_gfx_text(tx, 130U, "SAVE FAILED", kColorRed, kScreenBg, 3U);
            }

            g_t_timer = mc_millis();
            g_step = Step::ResultMessage;
            mc_dbg_363_touch_state = 7U;
            return TcalResult::Stay;
        }

        case Step::ResultMessage: {
            mc_dbg_363_touch_state = 7U;
            if (mc_time_elapsed_ms(g_t_timer, 1500U)) {
                mc_dbg_363_touch_state = 0U;
                return TcalResult::Done;
            }
            return TcalResult::Stay;
        }

        default:
            break;
    }

    return TcalResult::Stay;
}

}  // namespace mc
