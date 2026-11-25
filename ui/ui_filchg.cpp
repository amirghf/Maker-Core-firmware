// صفحه تعویض فیلامنت رابط کاربری (بند ۹.۱).
// هیچ هدر ST اینجا include نمی‌شود.
#include "ui_filchg.h"
#include "ui_home.h"
#include "ui_colors.h"
#include "ui_touch.h"
#include "../core/gfx.h"
#include "../core/text_format.h"
#include "../core/thermistor.h"
#include "../core/heater_ex.h"
#include "../core/filament_change.h"
#include "../core/filament_runout.h"
#include "../core/quick_stop.h"
#include "../core/encoder.h"
#include "../hal/hal_display.h"
#include "../hal/hal_buzzer.h"
#include "../hal/hal_time.h"
#include "../config/mc_config.h"

extern "C" {
extern volatile uint32_t mc_dbg_297_ui_fake_on;
extern volatile int32_t  mc_dbg_298_ui_fake_noz_x10;
extern volatile int32_t  mc_dbg_300_ui_fake_noz_tgt_x10;
}

namespace mc {
namespace {

constexpr uint16_t kSelBg      = mc_rgb565(24, 38, 64);
constexpr uint16_t kWarn       = mc_rgb565(255, 170, 0);
constexpr uint16_t kOrangeTgt  = mc_rgb565(255, 140, 0);   // رنگ نارنجی وضعیت گرم‌شدن (بند ۹.۱؛ همان رنگ ui_home.cpp)

static uint8_t  g_fc_sel          = 0U; // 0 = CONTINUE, 1 = STOP
static uint8_t  g_fc_step         = 0U; // گام نقاشی ورود (۰ تا ۵، ۲۵۵ = کار عادی)
static uint32_t g_last_refused_cnt = 0U;
static bool     g_not_det_msg     = false;
static uint32_t g_not_det_start_ms = 0U;

static uint8_t  g_cached_phase    = 255U;
static int32_t  g_cached_cur_x10  = -99999;
static int32_t  g_cached_tgt_x10  = -99999;
static bool     g_cached_can_res  = false;

const char* phase_to_str(uint8_t phase) {
    switch (phase) {
        case 1: return "HEATING";
        case 2: return "RETRACTING";
        case 3: return "PARKING";
        case 4: return "UNLOADING";
        case 5: return mc_filrun_caused() ? "FILAMENT RUNOUT - PRESS CONTINUE" : "CHANGE FILAMENT - PRESS CONTINUE";
        case 6: return "REHEATING";
        case 7: return "LOADING";
        case 8: return "PURGING";
        case 9: return "RETURNING";
        default: return "IDLE";
    }
}

static void draw_status_card() {
    constexpr uint16_t bx = 14U;
    constexpr uint16_t by = 46U;
    constexpr uint16_t bw = 452U;
    constexpr uint16_t bh = 56U;

    const uint8_t phase = mc_filchg_phase();
    const char* txt = phase_to_str(phase);

    mc_gfx_fill_rect(bx, by, bw, bh, kCardBg);
    mc_gfx_rect(bx, by, bw, bh, kCardBorder);

    uint16_t fg = kColorWhite;
    if (phase == 5U) {
        fg = kColorGreen;
    } else if ((phase == 1U) || (phase == 6U)) {
        fg = kOrangeTgt;
    }

    const uint16_t tw = mc_gfx_text_width(txt, 2U);
    const uint16_t tx = (tw < bw) ? static_cast<uint16_t>(bx + (bw - tw) / 2U) : static_cast<uint16_t>(bx + 6U);
    const uint16_t ty = static_cast<uint16_t>(by + (bh - 16U) / 2U);
    mc_gfx_text(tx, ty, txt, fg, kCardBg, 2U);

    g_cached_phase = phase;
}

static void draw_temp_card() {
    constexpr uint16_t bx = 14U;
    constexpr uint16_t by = 110U;
    constexpr uint16_t bw = 452U;
    constexpr uint16_t bh = 60U;

    int32_t cur_x10 = 0;
    bool sensor_ok = false;
    if (mc_dbg_297_ui_fake_on != 0U) {
        cur_x10 = mc_dbg_298_ui_fake_noz_x10;
        sensor_ok = (cur_x10 != -9999);
    } else {
        sensor_ok = mc_temp_get_x10(AdcChannel::TempExtruder, &cur_x10);
    }

    int32_t tgt_x10 = 0;
    if (mc_dbg_297_ui_fake_on != 0U) {
        tgt_x10 = mc_dbg_300_ui_fake_noz_tgt_x10;
    } else {
        tgt_x10 = mc_heater_ex_target_x10();
    }

    mc_gfx_fill_rect(bx, by, bw, bh, kCardBg);
    mc_gfx_rect(bx, by, bw, bh, kCardBorder);

    mc_gfx_text(24U, 120U, "NOZZLE TEMP", kLabelGray, kCardBg, 1U);

    char buf[32];
    if (!sensor_ok) {
        mc_fmt_text(buf, sizeof(buf), 0U, "--- / ");
    } else {
        const float val = static_cast<float>(cur_x10) / 10.0f;
        uint16_t p = mc_fmt_float(buf, sizeof(buf), 0U, val, 1U);
        (void)mc_fmt_text(buf, sizeof(buf), p, " / ");
    }

    uint16_t p = 0U;
    while (buf[p] != '\0') { ++p; }

    if (tgt_x10 <= 0) {
        (void)mc_fmt_text(buf, sizeof(buf), p, "OFF");
    } else {
        const uint32_t deg = static_cast<uint32_t>((tgt_x10 + 5) / 10);
        uint16_t p2 = mc_fmt_uint(buf, sizeof(buf), p, deg);
        (void)mc_fmt_text(buf, sizeof(buf), p2, " C");
    }

    mc_gfx_text(24U, 136U, buf, kColorWhite, kCardBg, 2U);

    mc_gfx_fill_rect(24U, 158U, 432U, 6U, kBarTrack);
    if (sensor_ok && (tgt_x10 > 0) && (cur_x10 > 0)) {
        int32_t fill_px = (cur_x10 * 432) / tgt_x10;
        if (fill_px < 0) { fill_px = 0; }
        if (fill_px > 432) { fill_px = 432; }
        if (fill_px > 0) {
            mc_gfx_fill_rect(24U, 158U, static_cast<uint16_t>(fill_px), 6U, kAccNozzle);
        }
    }

    g_cached_cur_x10 = cur_x10;
    g_cached_tgt_x10 = tgt_x10;
}

static void draw_not_det_msg(bool show) {
    constexpr uint16_t bx = 14U;
    constexpr uint16_t by = 180U;
    constexpr uint16_t bw = 452U;
    constexpr uint16_t bh = 42U;

    mc_gfx_fill_rect(bx, by, bw, bh, kScreenBg);
    if (show) {
        mc_gfx_fill_rect(bx, by, bw, bh, kCardBg);
        mc_gfx_rect(bx, by, bw, bh, kWarn);
        const uint16_t tw = mc_gfx_text_width("FILAMENT NOT DETECTED", 2U);
        const uint16_t tx = static_cast<uint16_t>(bx + (bw - tw) / 2U);
        const uint16_t ty = static_cast<uint16_t>(by + (bh - 16U) / 2U);
        mc_gfx_text(tx, ty, "FILAMENT NOT DETECTED", kWarn, kCardBg, 2U);
    }
}

static void draw_buttons() {
    const bool can_resume = mc_filchg_can_resume();

    // 0: CONTINUE button
    {
        constexpr uint16_t bx = 20U;
        constexpr uint16_t by = 236U;
        constexpr uint16_t bw = 210U;
        constexpr uint16_t bh = 56U;

        const bool focused = (g_fc_sel == 0U) && can_resume;
        uint16_t bg_col     = can_resume ? (focused ? kSelBg : kCardBg) : kCardBg;
        uint16_t border_col = can_resume ? (focused ? kAccentLine : kColorGreen) : kCardBorder;
        uint16_t fg_col     = can_resume ? kColorGreen : kLabelGray;

        mc_gfx_fill_rect(bx, by, bw, bh, bg_col);
        mc_gfx_rect(bx, by, bw, bh, border_col);
        if (focused) {
            mc_gfx_rect(static_cast<uint16_t>(bx + 1U), static_cast<uint16_t>(by + 1U),
                        static_cast<uint16_t>(bw - 2U), static_cast<uint16_t>(bh - 2U), border_col);
        }

        const uint16_t tw = mc_gfx_text_width("CONTINUE", 2U);
        const uint16_t tx = static_cast<uint16_t>(bx + (bw - tw) / 2U);
        const uint16_t ty = static_cast<uint16_t>(by + (bh - 16U) / 2U);
        mc_gfx_text(tx, ty, "CONTINUE", fg_col, bg_col, 2U);
    }

    // 1: STOP button
    {
        constexpr uint16_t bx = 250U;
        constexpr uint16_t by = 236U;
        constexpr uint16_t bw = 210U;
        constexpr uint16_t bh = 56U;

        const bool focused = (g_fc_sel == 1U) || (!can_resume);
        uint16_t bg_col     = focused ? mc_rgb565(50, 10, 10) : kCardBg;
        uint16_t border_col = focused ? kColorRed : kCardBorder;
        uint16_t fg_col     = kColorRed;

        mc_gfx_fill_rect(bx, by, bw, bh, bg_col);
        mc_gfx_rect(bx, by, bw, bh, border_col);
        if (focused) {
            mc_gfx_rect(static_cast<uint16_t>(bx + 1U), static_cast<uint16_t>(by + 1U),
                        static_cast<uint16_t>(bw - 2U), static_cast<uint16_t>(bh - 2U), border_col);
        }

        const uint16_t tw = mc_gfx_text_width("STOP", 2U);
        const uint16_t tx = static_cast<uint16_t>(bx + (bw - tw) / 2U);
        const uint16_t ty = static_cast<uint16_t>(by + (bh - 16U) / 2U);
        mc_gfx_text(tx, ty, "STOP", fg_col, bg_col, 2U);
    }

    g_cached_can_res = can_resume;
}

}  // namespace

void mc_ui_filchg_open() {
    g_fc_sel            = mc_filchg_can_resume() ? 0U : 1U;
    g_fc_step           = 0U;
    g_last_refused_cnt  = mc_dbg_439_fchg_resume_refused;
    g_not_det_msg       = false;
    g_not_det_start_ms  = 0U;

    g_cached_phase      = 255U;
    g_cached_cur_x10    = -99999;
    g_cached_tgt_x10    = -99999;
    g_cached_can_res    = false;

    (void)mc_encoder_take_clicks();
    (void)mc_encoder_take_press();
    mc_ui_touch_reset();
}

FilChgUiResult mc_ui_filchg_poll() {
    if (g_fc_step < 255U) {
        switch (g_fc_step) {
            case 0:
                mc_display_fill_screen(kScreenBg);
                g_fc_step = 1U;
                return FilChgUiResult::Stay;

            case 1: {
                mc_gfx_fill_rect(0U, 0U, 480U, 36U, kTopBarBg);
                mc_gfx_fill_rect(0U, 36U, 480U, 2U, kAccentLine);
                const uint16_t tw = mc_gfx_text_width("FILAMENT CHANGE", 2U);
                const uint16_t tx = static_cast<uint16_t>((480U - tw) / 2U);
                mc_gfx_text(tx, 10U, "FILAMENT CHANGE", kColorWhite, kTopBarBg, 2U);
                g_fc_step = 2U;
                return FilChgUiResult::Stay;
            }

            case 2:
                draw_status_card();
                g_fc_step = 3U;
                return FilChgUiResult::Stay;

            case 3:
                draw_temp_card();
                g_fc_step = 4U;
                return FilChgUiResult::Stay;

            case 4:
                draw_not_det_msg(false);
                g_fc_step = 5U;
                return FilChgUiResult::Stay;

            case 5:
                draw_buttons();
                g_fc_step = 255U;
                return FilChgUiResult::Stay;

            default:
                g_fc_step = 255U;
                return FilChgUiResult::Stay;
        }
    }

    // ۱) بررسی تغییرات رد شدن به‌خاطر سنسور
    if (mc_dbg_439_fchg_resume_refused != g_last_refused_cnt) {
        g_last_refused_cnt = mc_dbg_439_fchg_resume_refused;
        g_not_det_msg = true;
        g_not_det_start_ms = mc_millis();
        draw_not_det_msg(true);
        mc_buzzer_click(MC_UI_REFUSE_BEEP_MS);
    } else if (g_not_det_msg && mc_time_elapsed_ms(g_not_det_start_ms, static_cast<uint32_t>(MC_UI_SEN_MSG_MS))) {
        g_not_det_msg = false;
        draw_not_det_msg(false);
    }

    // ۲) ورودی لمس
    bool touch_enter = false;
    {
        const TouchGesture tg = mc_ui_touch_poll();
        if (tg.evt == TouchEvt::Tap) {
            if (mc_ui_touch_in_rect(tg.x, tg.y, 20U, 236U, 210U, 56U)) { // CONTINUE
                mc_ui_touch_accept(80U);
                if (mc_filchg_can_resume()) {
                    g_fc_sel = 0U;
                    touch_enter = true;
                }
            } else if (mc_ui_touch_in_rect(tg.x, tg.y, 250U, 236U, 210U, 56U)) { // STOP
                mc_ui_touch_accept(81U);
                g_fc_sel = 1U;
                touch_enter = true;
            }
        }
    }

    // ۳) ورودی انکودر
    const int32_t clicks = mc_encoder_take_clicks();
    if (clicks != 0) {
        if (mc_filchg_can_resume()) {
            g_fc_sel = (g_fc_sel == 0U) ? 1U : 0U;
            draw_buttons();
        }
    }

    const bool enc_enter = mc_encoder_take_press();
    if (touch_enter || enc_enter) {
        if (g_fc_sel == 0U) {
            if (mc_filchg_can_resume()) {
                mc_filchg_resume_request(1U); // src = 1 (دکمه/انکودر)
                mc_buzzer_click(MC_ENC_PRESS_BEEP_MS);
            }
        } else {
            mc_quick_stop();
            mc_buzzer_click(MC_UI_REFUSE_BEEP_MS);
        }
    }

    // ۴) به‌روزرسانی زنده
    if (mc_filchg_phase() != g_cached_phase) {
        draw_status_card();
    }

    if (mc_filchg_can_resume() != g_cached_can_res) {
        draw_buttons();
    }

    int32_t cur_x10 = 0;
    bool sensor_ok = false;
    if (mc_dbg_297_ui_fake_on != 0U) {
        cur_x10 = mc_dbg_298_ui_fake_noz_x10;
        sensor_ok = (cur_x10 != -9999);
    } else {
        sensor_ok = mc_temp_get_x10(AdcChannel::TempExtruder, &cur_x10);
    }

    int32_t tgt_x10 = 0;
    if (mc_dbg_297_ui_fake_on != 0U) {
        tgt_x10 = mc_dbg_300_ui_fake_noz_tgt_x10;
    } else {
        tgt_x10 = mc_heater_ex_target_x10();
    }

    if ((cur_x10 != g_cached_cur_x10) || (tgt_x10 != g_cached_tgt_x10)) {
        draw_temp_card();
    }

    return FilChgUiResult::Stay;
}

}  // namespace mc
