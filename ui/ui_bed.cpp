// صفحه داغ رابط کاربری (بند ۸.۱۰).
// هیچ هدر ST اینجا include نمی‌شود. فقط از hal و gfx و encoder و thermistor و heater_bed استفاده می‌کند.
#include "ui_bed.h"
#include "ui_home.h"
#include "ui_colors.h"
#include "ui_touch.h"
#include "../core/gfx.h"
#include "../core/text_format.h"
#include "../core/thermistor.h"
#include "../core/heater_bed.h"
#include "../core/encoder.h"
#include "../hal/hal_display.h"
#include "../config/mc_config.h"

extern "C" {
volatile uint32_t mc_dbg_340_bed_sel_shown = 3;   // خواندنی: اندیس مورد فوکوس‌شده (۰ تا ۳)
volatile uint32_t mc_dbg_341_bed_enter_cnt = 0;   // خواندنی: تعداد اینترها روی مورد غیر BACK
volatile uint32_t mc_dbg_342_bed_enter_idx = 255; // خواندنی: اندیس مورد آخرین اینتر (۲۵۵ = هنوز نشده)
volatile uint32_t mc_dbg_343_bed_force     = 0;   // نوشتنی: ۰ = بی‌اثر؛ ۱ = باز کردن صفحه داغ؛ ۲ = برگشت به منو؛ ۳ تا ۶ = فوکوس روی مورد (مقدار - ۳)
}

namespace mc {
namespace {

constexpr uint16_t kSelBg    = mc_rgb565(24, 38, 64);
constexpr uint16_t kCoolBlue = mc_rgb565(60, 140, 255);

static uint8_t g_bed_sel        = 3U; // فوکوس اولیه روی BACK (اندیس ۳)
static uint8_t g_bed_step       = 0U; // گام نقاشی ورود (۰ تا ۴، ۲۵۵ = کار عادی)
static uint8_t g_bed_live_cycle = 0U; // چرخه‌گیری به‌روزرسانی ۲ بخش در حالت عادی (۰ تا ۱)

static int32_t g_cached_cur_temp_x10 = -99999;
static int32_t g_cached_tgt_temp_x10 = -99999;
static uint8_t g_cached_badge_code   = 255U;
static bool    g_cached_sensor_ok    = false;

static bool    g_cached_heat1_active = false;
static bool    g_cached_heat2_active = false;

static void draw_temp_group() {
    int32_t cur_temp_x10 = 0;
    bool sensor_ok = false;
    const bool fake = (mc_dbg_297_ui_fake_on != 0U);
    if (fake) {
        cur_temp_x10 = mc_dbg_299_ui_fake_bed_x10;
        sensor_ok = (cur_temp_x10 != -9999);
    } else {
        sensor_ok = mc_temp_get_x10(AdcChannel::TempBed, &cur_temp_x10);
    }

    int32_t tgt_temp_x10 = 0;
    if (fake) {
        tgt_temp_x10 = mc_dbg_301_ui_fake_bed_tgt_x10;
    } else {
        tgt_temp_x10 = mc_heater_bed_target_x10();
    }

    mc_gfx_text(20U, 48U, "BED", kLabelGray, kScreenBg, 1U);

    mc_gfx_fill_rect(20U, 62U, 110U, 22U, kScreenBg);
    if (!sensor_ok) {
        mc_gfx_text(20U, 62U, "---", kColorRed, kScreenBg, 3U);
    } else {
        char t_str[16];
        const float val = static_cast<float>(cur_temp_x10) / 10.0f;
        mc_fmt_float(t_str, sizeof(t_str), 0U, val, 1U);
        mc_gfx_text(20U, 62U, t_str, kColorWhite, kScreenBg, 3U);
    }

    mc_gfx_fill_rect(124U, 69U, 177U, 15U, kScreenBg);
    if (tgt_temp_x10 <= 0) {
        mc_gfx_text(124U, 69U, "/ OFF", kLabelGray, kScreenBg, 2U);
    } else {
        const uint32_t deg = static_cast<uint32_t>((tgt_temp_x10 + 5) / 10);
        char tgt_buf[16];
        char num_buf[10];
        mc_fmt_uint(num_buf, sizeof(num_buf), 0U, deg);
        mc_fmt_text(tgt_buf, sizeof(tgt_buf), 0U, "/ ");
        uint16_t pos = 2U;
        for (uint16_t i = 0U; num_buf[i] != '\0'; ++i) {
            tgt_buf[pos++] = num_buf[i];
        }
        tgt_buf[pos++] = ' ';
        tgt_buf[pos++] = 'C';
        tgt_buf[pos] = '\0';
        mc_gfx_text(124U, 69U, tgt_buf, kLabelGray, kScreenBg, 2U);
    }

    uint8_t badge_code = 4U;
    const char* badge_text = "READY";
    uint16_t badge_bg = kColorGreen;
    uint16_t badge_fg = kScreenBg;

    if (!sensor_ok) {
        badge_code = 0U;
        badge_text = "ERROR";
        badge_bg = kColorRed;
        badge_fg = kColorWhite;
    } else if (tgt_temp_x10 <= 0) {
        badge_code = 1U;
        badge_text = "OFF";
        badge_bg = kCardBg;
        badge_fg = kLabelGray;
    } else if (cur_temp_x10 < (tgt_temp_x10 - MC_UI_TEMP_NEAR_X10)) {
        badge_code = 2U;
        badge_text = "HEATING";
        badge_bg = kYellowBusy;
        badge_fg = kScreenBg;
    } else if (cur_temp_x10 > (tgt_temp_x10 + MC_UI_TEMP_NEAR_X10)) {
        badge_code = 3U;
        badge_text = "COOLING";
        badge_bg = kCoolBlue;
        badge_fg = kColorWhite;
    } else {
        badge_code = 4U;
        badge_text = "READY";
        badge_bg = kColorGreen;
        badge_fg = kScreenBg;
    }

    mc_gfx_fill_rect(352U, 52U, 112U, 26U, badge_bg);
    const uint16_t btw = mc_gfx_text_width(badge_text, 2U);
    const uint16_t btx = static_cast<uint16_t>(352U + (112U - btw) / 2U);
    constexpr uint16_t bty = 57U;
    mc_gfx_text(btx, bty, badge_text, badge_fg, badge_bg, 2U);

    mc_gfx_fill_rect(20U, 94U, 444U, 6U, kBarTrack);
    if (sensor_ok && (tgt_temp_x10 > 0) && (cur_temp_x10 > 0)) {
        int32_t fill_px = (cur_temp_x10 * 444) / tgt_temp_x10;
        if (fill_px < 0) { fill_px = 0; }
        if (fill_px > 444) { fill_px = 444; }
        if (fill_px > 0) {
            mc_gfx_fill_rect(20U, 94U, static_cast<uint16_t>(fill_px), 6U, kAccBed);
        }
    }

    mc_gfx_hline(14U, 106U, 452U, kDividerGray);

    g_cached_cur_temp_x10 = cur_temp_x10;
    g_cached_tgt_temp_x10 = tgt_temp_x10;
    g_cached_badge_code   = badge_code;
    g_cached_sensor_ok    = sensor_ok;
}

static void draw_bed_item(uint8_t item_idx) {
    if (item_idx >= 4U) { return; }
    const bool focused = (item_idx == g_bed_sel);

    int32_t tgt_temp_x10 = 0;
    if (mc_dbg_297_ui_fake_on != 0U) {
        tgt_temp_x10 = mc_dbg_301_ui_fake_bed_tgt_x10;
    } else {
        tgt_temp_x10 = mc_heater_bed_target_x10();
    }

    if (item_idx == 0U) { // HEAT Preset 1
        constexpr uint16_t bx = 8U;
        constexpr uint16_t by = 130U;
        constexpr uint16_t bw = 150U;
        constexpr uint16_t bh = 56U;

        const bool is_active = (tgt_temp_x10 == (MC_UI_BED_PRESET_1_C * 10));

        const uint16_t bg_col     = focused ? kSelBg : kCardBg;
        const uint16_t border_col = focused ? kAccentLine : (is_active ? kAccBed : kCardBorder);
        const uint16_t fg_col     = is_active ? kAccBed : (focused ? kColorWhite : kLabelGray);

        mc_gfx_fill_rect(bx, by, bw, bh, bg_col);
        mc_gfx_rect(bx, by, bw, bh, border_col);
        if (focused) {
            mc_gfx_rect(static_cast<uint16_t>(bx + 1U), static_cast<uint16_t>(by + 1U),
                        static_cast<uint16_t>(bw - 2U), static_cast<uint16_t>(bh - 2U), border_col);
        }

        char text_buf[16];
        mc_fmt_text(text_buf, sizeof(text_buf), 0U, "HEAT ");
        mc_fmt_uint(text_buf, sizeof(text_buf), 5U, MC_UI_BED_PRESET_1_C);

        const uint16_t tw = mc_gfx_text_width(text_buf, 2U);
        const uint16_t tx = static_cast<uint16_t>(bx + (bw - tw) / 2U);
        const uint16_t ty = static_cast<uint16_t>(by + (bh - 16U) / 2U);
        mc_gfx_text(tx, ty, text_buf, fg_col, bg_col, 2U);

        g_cached_heat1_active = is_active;
    } else if (item_idx == 1U) { // HEAT Preset 2
        constexpr uint16_t bx = 166U;
        constexpr uint16_t by = 130U;
        constexpr uint16_t bw = 150U;
        constexpr uint16_t bh = 56U;

        const bool is_active = (tgt_temp_x10 == (MC_UI_BED_PRESET_2_C * 10));

        const uint16_t bg_col     = focused ? kSelBg : kCardBg;
        const uint16_t border_col = focused ? kAccentLine : (is_active ? kAccBed : kCardBorder);
        const uint16_t fg_col     = is_active ? kAccBed : (focused ? kColorWhite : kLabelGray);

        mc_gfx_fill_rect(bx, by, bw, bh, bg_col);
        mc_gfx_rect(bx, by, bw, bh, border_col);
        if (focused) {
            mc_gfx_rect(static_cast<uint16_t>(bx + 1U), static_cast<uint16_t>(by + 1U),
                        static_cast<uint16_t>(bw - 2U), static_cast<uint16_t>(bh - 2U), border_col);
        }

        char text_buf[16];
        mc_fmt_text(text_buf, sizeof(text_buf), 0U, "HEAT ");
        mc_fmt_uint(text_buf, sizeof(text_buf), 5U, MC_UI_BED_PRESET_2_C);

        const uint16_t tw = mc_gfx_text_width(text_buf, 2U);
        const uint16_t tx = static_cast<uint16_t>(bx + (bw - tw) / 2U);
        const uint16_t ty = static_cast<uint16_t>(by + (bh - 16U) / 2U);
        mc_gfx_text(tx, ty, text_buf, fg_col, bg_col, 2U);

        g_cached_heat2_active = is_active;
    } else if (item_idx == 2U) { // COOL
        constexpr uint16_t bx = 324U;
        constexpr uint16_t by = 130U;
        constexpr uint16_t bw = 148U;
        constexpr uint16_t bh = 56U;

        const uint16_t bg_col     = focused ? kSelBg : kCardBg;
        const uint16_t fg_col     = focused ? kColorWhite : kLabelGray;
        const uint16_t border_col = focused ? kAccentLine : kCardBorder;

        mc_gfx_fill_rect(bx, by, bw, bh, bg_col);
        mc_gfx_rect(bx, by, bw, bh, border_col);
        if (focused) {
            mc_gfx_rect(static_cast<uint16_t>(bx + 1U), static_cast<uint16_t>(by + 1U),
                        static_cast<uint16_t>(bw - 2U), static_cast<uint16_t>(bh - 2U), border_col);
        }

        const uint16_t tw = mc_gfx_text_width("COOL", 2U);
        const uint16_t tx = static_cast<uint16_t>(bx + (bw - tw) / 2U);
        const uint16_t ty = static_cast<uint16_t>(by + (bh - 16U) / 2U);
        mc_gfx_text(tx, ty, "COOL", fg_col, bg_col, 2U);
    } else if (item_idx == 3U) { // BACK Button
        constexpr uint16_t bx = 324U;
        constexpr uint16_t by = 258U;
        constexpr uint16_t bw = 148U;
        constexpr uint16_t bh = 44U;

        const uint16_t bg_col     = focused ? kSelBg : kCardBg;
        const uint16_t fg_col     = focused ? kColorWhite : kLabelGray;
        const uint16_t border_col = focused ? kAccentLine : kCardBorder;

        mc_gfx_fill_rect(bx, by, bw, bh, bg_col);
        mc_gfx_rect(bx, by, bw, bh, border_col);
        if (focused) {
            mc_gfx_rect(static_cast<uint16_t>(bx + 1U), static_cast<uint16_t>(by + 1U),
                        static_cast<uint16_t>(bw - 2U), static_cast<uint16_t>(bh - 2U), border_col);
        }

        const uint16_t tw = mc_gfx_text_width("BACK", 2U);
        const uint16_t tx = static_cast<uint16_t>(bx + (bw - tw) / 2U);
        const uint16_t ty = static_cast<uint16_t>(by + (bh - 16U) / 2U);
        mc_gfx_text(tx, ty, "BACK", fg_col, bg_col, 2U);
    }
}

static void apply_bed_selection(uint8_t target_sel) {
    if (target_sel >= 4U) { return; }
    const uint8_t old_sel = g_bed_sel;
    g_bed_sel = target_sel;
    mc_dbg_340_bed_sel_shown = g_bed_sel;

    if (old_sel != g_bed_sel) {
        draw_bed_item(old_sel);
        draw_bed_item(g_bed_sel);
    }
}

}  // namespace

void mc_ui_bed_open() {
    g_bed_sel        = 3U; // فوکوس اولیه روی BACK (اندیس ۳)
    g_bed_step       = 0U;
    g_bed_live_cycle = 0U;

    g_cached_cur_temp_x10 = -99999;
    g_cached_tgt_temp_x10 = -99999;
    g_cached_badge_code   = 255U;
    g_cached_sensor_ok    = false;

    g_cached_heat1_active = false;
    g_cached_heat2_active = false;

    mc_dbg_340_bed_sel_shown = 3U;
    mc_dbg_341_bed_enter_cnt = 0U;
    mc_dbg_342_bed_enter_idx = 255U;
    mc_dbg_343_bed_force     = 0U;

    (void)mc_encoder_take_clicks();
    (void)mc_encoder_take_press();
    mc_ui_touch_reset();
}

BedResult mc_ui_bed_poll() {
    (void)mc_dbg_340_bed_sel_shown;
    (void)mc_dbg_341_bed_enter_cnt;
    (void)mc_dbg_342_bed_enter_idx;
    (void)mc_dbg_343_bed_force;

    if (mc_dbg_343_bed_force != 0U) {
        const uint32_t force_val = mc_dbg_343_bed_force;
        mc_dbg_343_bed_force = 0U;

        if (force_val == 2U) {
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return BedResult::Back;
        } else if ((force_val >= 3U) && (force_val <= 6U)) {
            const uint8_t target_sel = static_cast<uint8_t>(force_val - 3U);
            if (target_sel != g_bed_sel) {
                apply_bed_selection(target_sel);
            }
        }
    }

    if (g_bed_step < 255U) {
        switch (g_bed_step) {
            case 0:
                mc_display_fill_screen(kScreenBg);
                g_bed_step = 1U;
                return BedResult::Stay;

            case 1: {
                mc_gfx_fill_rect(0U, 0U, 480U, 36U, kTopBarBg);
                mc_gfx_fill_rect(0U, 36U, 480U, 2U, kAccentLine);
                const uint16_t tw = mc_gfx_text_width("HOT BED", 2U);
                const uint16_t tx = static_cast<uint16_t>((480U - tw) / 2U);
                mc_gfx_text(tx, 10U, "HOT BED", kColorWhite, kTopBarBg, 2U);
                g_bed_step = 2U;
                return BedResult::Stay;
            }

            case 2:
                draw_temp_group();
                g_bed_step = 3U;
                return BedResult::Stay;

            case 3:
                draw_bed_item(0U);
                draw_bed_item(1U);
                draw_bed_item(2U);
                g_bed_step = 4U;
                return BedResult::Stay;

            case 4:
                draw_bed_item(3U);
                g_bed_step = 255U;
                return BedResult::Stay;

            default:
                g_bed_step = 255U;
                return BedResult::Stay;
        }
    }

    // لمس (بند ۸.۱۶): ضربه روی دکمه BACK مثل اینتر روی BACK است
    {
        const TouchGesture tg = mc_ui_touch_poll();
        if ((tg.evt == TouchEvt::Tap) && mc_ui_touch_in_rect(tg.x, tg.y, 324U, 258U, 148U, 44U)) {
            mc_ui_touch_accept(20U);
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return BedResult::Back;
        }
    }

    const int32_t clicks = mc_encoder_take_clicks();
    if (clicks != 0) {
        int32_t new_idx = static_cast<int32_t>(g_bed_sel) + clicks;
        while (new_idx < 0) {
            new_idx += 4;
        }
        while (new_idx >= 4) {
            new_idx %= 4;
        }
        const uint8_t target_sel = static_cast<uint8_t>(new_idx);
        if (target_sel != g_bed_sel) {
            apply_bed_selection(target_sel);
        }
    }

    if (mc_encoder_take_press()) {
        if (g_bed_sel == 3U) { // BACK
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return BedResult::Back;
        } else {
            mc_dbg_341_bed_enter_cnt = mc_dbg_341_bed_enter_cnt + 1U;
            mc_dbg_342_bed_enter_idx = g_bed_sel;
        }
    }

    const uint8_t cycle = g_bed_live_cycle;
    g_bed_live_cycle = static_cast<uint8_t>((g_bed_live_cycle + 1U) % 2U);

    if (cycle == 0U) { // Temp Group
        int32_t cur_temp_x10 = 0;
        bool sensor_ok = false;
        const bool fake = (mc_dbg_297_ui_fake_on != 0U);
        if (fake) {
            cur_temp_x10 = mc_dbg_299_ui_fake_bed_x10;
            sensor_ok = (cur_temp_x10 != -9999);
        } else {
            sensor_ok = mc_temp_get_x10(AdcChannel::TempBed, &cur_temp_x10);
        }

        int32_t tgt_temp_x10 = 0;
        if (fake) {
            tgt_temp_x10 = mc_dbg_301_ui_fake_bed_tgt_x10;
        } else {
            tgt_temp_x10 = mc_heater_bed_target_x10();
        }

        uint8_t badge_code = 4U;
        if (!sensor_ok) {
            badge_code = 0U;
        } else if (tgt_temp_x10 <= 0) {
            badge_code = 1U;
        } else if (cur_temp_x10 < (tgt_temp_x10 - MC_UI_TEMP_NEAR_X10)) {
            badge_code = 2U;
        } else if (cur_temp_x10 > (tgt_temp_x10 + MC_UI_TEMP_NEAR_X10)) {
            badge_code = 3U;
        } else {
            badge_code = 4U;
        }

        if ((sensor_ok != g_cached_sensor_ok) ||
            (cur_temp_x10 != g_cached_cur_temp_x10) ||
            (tgt_temp_x10 != g_cached_tgt_temp_x10) ||
            (badge_code != g_cached_badge_code)) {
            draw_temp_group();
        }
    } else if (cycle == 1U) { // HEAT buttons active state
        int32_t tgt_temp_x10 = 0;
        if (mc_dbg_297_ui_fake_on != 0U) {
            tgt_temp_x10 = mc_dbg_301_ui_fake_bed_tgt_x10;
        } else {
            tgt_temp_x10 = mc_heater_bed_target_x10();
        }
        const bool h1_act = (tgt_temp_x10 == (MC_UI_BED_PRESET_1_C * 10));
        const bool h2_act = (tgt_temp_x10 == (MC_UI_BED_PRESET_2_C * 10));
        if (h1_act != g_cached_heat1_active) {
            draw_bed_item(0U);
        }
        if (h2_act != g_cached_heat2_active) {
            draw_bed_item(1U);
        }
    }

    return BedResult::Stay;
}

}  // namespace mc
