// صفحه اکسترودر رابط کاربری (بند ۸.۹).
// هیچ هدر ST اینجا include نمی‌شود. فقط از hal و gfx و encoder و thermistor و heater_ex و extruder و output_pwm استفاده می‌کند.
#include "ui_extruder.h"
#include "ui_home.h"
#include "ui_colors.h"
#include "../core/gfx.h"
#include "../core/text_format.h"
#include "../core/thermistor.h"
#include "../core/heater_ex.h"
#include "../core/extruder.h"
#include "../core/output_pwm.h"
#include "../core/encoder.h"
#include "../hal/hal_display.h"
#include "../config/mc_config.h"

extern "C" {
volatile uint32_t mc_dbg_334_ex_sel_shown  = 7;   // خواندنی: اندیس مورد فوکوس‌شده (۰ تا ۷)
volatile uint32_t mc_dbg_335_ex_enter_cnt  = 0;   // خواندنی: تعداد اینترها روی مورد غیر BACK و غیر چیپ‌ها
volatile uint32_t mc_dbg_336_ex_enter_idx  = 255; // خواندنی: اندیس مورد آخرین اینتر (۲۵۵ = هنوز نشده)
volatile uint32_t mc_dbg_337_ex_force      = 0;   // نوشتنی: ۰ = بی‌اثر؛ ۱ = باز کردن صفحه اکسترودر؛ ۲ = برگشت به منو؛ ۳ تا ۱۰ = فوکوس روی مورد (مقدار - ۳)
volatile uint32_t mc_dbg_338_ex_feed_shown = 1;   // خواندنی: اندیس چیپ گام نمایشی (۰ تا ۲)
volatile uint32_t mc_dbg_339_ex_fake_rule  = 0;   // نوشتنی: ۰ = وضعیت واقعی HOT RULE؛ ۱ = نمایش «روشن»؛ ۲ = نمایش «خاموش»
}

namespace mc {
namespace {

constexpr uint16_t kSelBg    = mc_rgb565(24, 38, 64);
constexpr uint16_t kWarn     = mc_rgb565(255, 170, 0);
constexpr uint16_t kCoolBlue = mc_rgb565(60, 140, 255);

static uint8_t g_ex_sel        = 7U; // فوکوس اولیه روی BACK (اندیس ۷)
static uint8_t g_ex_feed_step  = MC_UI_EX_FEED_STEP_DEFAULT_IDX; // چیپ فعال پیش‌فرض
static uint8_t g_ex_step       = 0U; // گام نقاشی ورود (۰ تا ۶، ۲۵۵ = کار عادی)
static uint8_t g_ex_live_cycle = 0U; // چرخه‌گیری به‌روزرسانی ۴ بخش در حالت عادی (۰ تا ۳)

static int32_t g_cached_cur_temp_x10 = -99999;
static int32_t g_cached_tgt_temp_x10 = -99999;
static uint8_t g_cached_badge_code   = 255U;
static bool    g_cached_sensor_ok    = false;

static bool    g_cached_heat_active  = false;
static bool    g_cached_rule_on      = false;
static bool    g_cached_fan_on       = false;

static void draw_dot(uint16_t cx, uint16_t cy, uint16_t color) {
    mc_gfx_fill_rect(static_cast<uint16_t>(cx - 2U), static_cast<uint16_t>(cy - 4U), 5U, 9U, color);
    mc_gfx_fill_rect(static_cast<uint16_t>(cx - 3U), static_cast<uint16_t>(cy - 3U), 7U, 7U, color);
    mc_gfx_fill_rect(static_cast<uint16_t>(cx - 4U), static_cast<uint16_t>(cy - 2U), 9U, 5U, color);
}

static void draw_temp_group() {
    int32_t cur_temp_x10 = 0;
    bool sensor_ok = false;
    const bool fake = (mc_dbg_297_ui_fake_on != 0U);
    if (fake) {
        cur_temp_x10 = mc_dbg_298_ui_fake_noz_x10;
        sensor_ok = (cur_temp_x10 != -9999);
    } else {
        sensor_ok = mc_temp_get_x10(AdcChannel::TempExtruder, &cur_temp_x10);
    }

    int32_t tgt_temp_x10 = 0;
    if (fake) {
        tgt_temp_x10 = mc_dbg_300_ui_fake_noz_tgt_x10;
    } else {
        tgt_temp_x10 = mc_heater_ex_target_x10();
    }

    mc_gfx_text(20U, 48U, "NOZZLE", kLabelGray, kScreenBg, 1U);

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
            mc_gfx_fill_rect(20U, 94U, static_cast<uint16_t>(fill_px), 6U, kAccNozzle);
        }
    }

    mc_gfx_hline(14U, 106U, 452U, kDividerGray);

    g_cached_cur_temp_x10 = cur_temp_x10;
    g_cached_tgt_temp_x10 = tgt_temp_x10;
    g_cached_badge_code   = badge_code;
    g_cached_sensor_ok    = sensor_ok;
}

static void draw_ex_item(uint8_t item_idx) {
    if (item_idx >= 8U) { return; }
    const bool focused = (item_idx == g_ex_sel);

    if (item_idx == 0U) { // HEAT
        constexpr uint16_t bx = 190U;
        constexpr uint16_t by = 118U;
        constexpr uint16_t bw = 146U;
        constexpr uint16_t bh = 32U;

        int32_t tgt_temp_x10 = 0;
        if (mc_dbg_297_ui_fake_on != 0U) {
            tgt_temp_x10 = mc_dbg_300_ui_fake_noz_tgt_x10;
        } else {
            tgt_temp_x10 = mc_heater_ex_target_x10();
        }
        const bool is_active = (tgt_temp_x10 == (MC_UI_EX_HEAT_TEMP_C * 10));

        const uint16_t bg_col     = focused ? kSelBg : kCardBg;
        const uint16_t border_col = focused ? kAccentLine : (is_active ? kAccNozzle : kCardBorder);
        const uint16_t fg_col     = is_active ? kAccNozzle : (focused ? kColorWhite : kLabelGray);

        mc_gfx_fill_rect(bx, by, bw, bh, bg_col);
        mc_gfx_rect(bx, by, bw, bh, border_col);
        if (focused) {
            mc_gfx_rect(static_cast<uint16_t>(bx + 1U), static_cast<uint16_t>(by + 1U),
                        static_cast<uint16_t>(bw - 2U), static_cast<uint16_t>(bh - 2U), border_col);
        }

        char text_buf[16];
        mc_fmt_text(text_buf, sizeof(text_buf), 0U, "HEAT ");
        mc_fmt_uint(text_buf, sizeof(text_buf), 5U, MC_UI_EX_HEAT_TEMP_C);

        const uint16_t tw = mc_gfx_text_width(text_buf, 2U);
        const uint16_t tx = static_cast<uint16_t>(bx + (bw - tw) / 2U);
        const uint16_t ty = static_cast<uint16_t>(by + (bh - 16U) / 2U);
        mc_gfx_text(tx, ty, text_buf, fg_col, bg_col, 2U);

        g_cached_heat_active = is_active;
    } else if (item_idx == 1U) { // COOL
        constexpr uint16_t bx = 344U;
        constexpr uint16_t by = 118U;
        constexpr uint16_t bw = 120U;
        constexpr uint16_t bh = 32U;

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
    } else if (item_idx == 2U) { // FEED Minus
        constexpr uint16_t bx = 190U;
        constexpr uint16_t by = 164U;
        constexpr uint16_t bw = 70U;
        constexpr uint16_t bh = 32U;

        const uint16_t bg_col     = focused ? kSelBg : kCardBg;
        const uint16_t fg_col     = focused ? kColorWhite : kLabelGray;
        const uint16_t border_col = focused ? kAccentLine : kCardBorder;

        mc_gfx_fill_rect(bx, by, bw, bh, bg_col);
        mc_gfx_rect(bx, by, bw, bh, border_col);
        if (focused) {
            mc_gfx_rect(static_cast<uint16_t>(bx + 1U), static_cast<uint16_t>(by + 1U),
                        static_cast<uint16_t>(bw - 2U), static_cast<uint16_t>(bh - 2U), border_col);
        }

        const uint16_t cx = static_cast<uint16_t>(bx + (bw - 16U) / 2U);
        const uint16_t cy = static_cast<uint16_t>(by + (bh - 3U) / 2U);
        mc_gfx_fill_rect(cx, cy, 16U, 3U, fg_col);
    } else if (item_idx == 3U) { // FEED Plus
        constexpr uint16_t bx = 268U;
        constexpr uint16_t by = 164U;
        constexpr uint16_t bw = 70U;
        constexpr uint16_t bh = 32U;

        const uint16_t bg_col     = focused ? kSelBg : kCardBg;
        const uint16_t fg_col     = focused ? kColorWhite : kLabelGray;
        const uint16_t border_col = focused ? kAccentLine : kCardBorder;

        mc_gfx_fill_rect(bx, by, bw, bh, bg_col);
        mc_gfx_rect(bx, by, bw, bh, border_col);
        if (focused) {
            mc_gfx_rect(static_cast<uint16_t>(bx + 1U), static_cast<uint16_t>(by + 1U),
                        static_cast<uint16_t>(bw - 2U), static_cast<uint16_t>(bh - 2U), border_col);
        }

        const uint16_t cx_h = static_cast<uint16_t>(bx + (bw - 16U) / 2U);
        const uint16_t cy_h = static_cast<uint16_t>(by + (bh - 3U) / 2U);
        mc_gfx_fill_rect(cx_h, cy_h, 16U, 3U, fg_col);

        const uint16_t cx_v = static_cast<uint16_t>(bx + (bw - 3U) / 2U);
        const uint16_t cy_v = static_cast<uint16_t>(by + (bh - 16U) / 2U);
        mc_gfx_fill_rect(cx_v, cy_v, 3U, 16U, fg_col);
    } else if (item_idx == 4U) { // FEED Chips
        static const uint16_t kChipX[3] = { 346U, 386U, 426U };
        static const uint32_t kChipSteps[3] = {
            MC_UI_EX_FEED_STEP_1_MM,
            MC_UI_EX_FEED_STEP_2_MM,
            MC_UI_EX_FEED_STEP_3_MM
        };

        for (uint8_t c = 0U; c < 3U; ++c) {
            const bool is_active = (c == g_ex_feed_step);
            const uint16_t cx = kChipX[c];
            constexpr uint16_t cy = 164U;
            constexpr uint16_t cw = 36U;
            constexpr uint16_t ch = 32U;

            char chip_str[10];
            mc_fmt_uint(chip_str, sizeof(chip_str), 0U, kChipSteps[c]);

            if (is_active) {
                mc_gfx_fill_rect(cx, cy, cw, ch, kAccentLine);
                if (focused) {
                    mc_gfx_rect(cx, cy, cw, ch, kColorWhite);
                    mc_gfx_rect(static_cast<uint16_t>(cx + 1U), static_cast<uint16_t>(cy + 1U),
                                static_cast<uint16_t>(cw - 2U), static_cast<uint16_t>(ch - 2U), kColorWhite);
                }
                const uint16_t tw = mc_gfx_text_width(chip_str, 2U);
                const uint16_t tx = static_cast<uint16_t>(cx + (cw - tw) / 2U);
                const uint16_t ty = static_cast<uint16_t>(cy + (ch - 16U) / 2U);
                mc_gfx_text(tx, ty, chip_str, kScreenBg, kAccentLine, 2U);
            } else {
                mc_gfx_fill_rect(cx, cy, cw, ch, kCardBg);
                mc_gfx_rect(cx, cy, cw, ch, kCardBorder);
                const uint16_t tw = mc_gfx_text_width(chip_str, 2U);
                const uint16_t tx = static_cast<uint16_t>(cx + (cw - tw) / 2U);
                const uint16_t ty = static_cast<uint16_t>(cy + (ch - 16U) / 2U);
                mc_gfx_text(tx, ty, chip_str, kLabelGray, kCardBg, 2U);
            }
        }
    } else if (item_idx == 5U) { // HOT RULE Card
        constexpr uint16_t bx = 8U;
        constexpr uint16_t by = 206U;
        constexpr uint16_t bw = 226U;
        constexpr uint16_t bh = 40U;

        const uint16_t bg_col     = focused ? kSelBg : kCardBg;
        const uint16_t border_col = focused ? kAccentLine : kCardBorder;

        mc_gfx_fill_rect(bx, by, bw, bh, bg_col);
        mc_gfx_rect(bx, by, bw, bh, border_col);
        if (focused) {
            mc_gfx_rect(static_cast<uint16_t>(bx + 1U), static_cast<uint16_t>(by + 1U),
                        static_cast<uint16_t>(bw - 2U), static_cast<uint16_t>(bh - 2U), border_col);
        }

        mc_gfx_text(22U, 219U, "HOT RULE", kLabelGray, bg_col, 2U);

        bool rule_on = false;
        if (mc_dbg_339_ex_fake_rule == 1U) {
            rule_on = true;
        } else if (mc_dbg_339_ex_fake_rule == 2U) {
            rule_on = false;
        } else {
            rule_on = !mc_extruder_cold_ok_get();
        }

        const char* st_text = rule_on ? "ON" : "OFF";
        const uint16_t st_color = rule_on ? kColorGreen : kWarn;

        const uint16_t tw = mc_gfx_text_width(st_text, 2U);
        const uint16_t tx = static_cast<uint16_t>(226U - tw);
        mc_gfx_text(tx, 219U, st_text, st_color, bg_col, 2U);

        const uint16_t dot_cx = static_cast<uint16_t>(tx - 16U);
        draw_dot(dot_cx, 226U, st_color);

        g_cached_rule_on = rule_on;
    } else if (item_idx == 6U) { // FAN Card
        constexpr uint16_t bx = 246U;
        constexpr uint16_t by = 206U;
        constexpr uint16_t bw = 226U;
        constexpr uint16_t bh = 40U;

        const uint16_t bg_col     = focused ? kSelBg : kCardBg;
        const uint16_t border_col = focused ? kAccentLine : kCardBorder;

        mc_gfx_fill_rect(bx, by, bw, bh, bg_col);
        mc_gfx_rect(bx, by, bw, bh, border_col);
        if (focused) {
            mc_gfx_rect(static_cast<uint16_t>(bx + 1U), static_cast<uint16_t>(by + 1U),
                        static_cast<uint16_t>(bw - 2U), static_cast<uint16_t>(bh - 2U), border_col);
        }

        mc_gfx_text(260U, 219U, "FAN 100%", kLabelGray, bg_col, 2U);

        bool fan_on = false;
        if (mc_dbg_297_ui_fake_on != 0U) {
            fan_on = (mc_dbg_304_ui_fake_fan_pct > 0U);
        } else {
            fan_on = (mc_fan_get() > 0U);
        }

        const char* st_text = fan_on ? "ON" : "OFF";
        const uint16_t st_color = fan_on ? kAccFan : kLabelGray;

        const uint16_t tw = mc_gfx_text_width(st_text, 2U);
        const uint16_t tx = static_cast<uint16_t>(464U - tw);
        mc_gfx_text(tx, 219U, st_text, st_color, bg_col, 2U);

        const uint16_t dot_cx = static_cast<uint16_t>(tx - 16U);
        draw_dot(dot_cx, 226U, st_color);

        g_cached_fan_on = fan_on;
    } else if (item_idx == 7U) { // BACK Button
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

static void apply_ex_selection(uint8_t target_sel) {
    if (target_sel >= 8U) { return; }
    const uint8_t old_sel = g_ex_sel;
    g_ex_sel = target_sel;
    mc_dbg_334_ex_sel_shown = g_ex_sel;

    if (old_sel != g_ex_sel) {
        draw_ex_item(old_sel);
        draw_ex_item(g_ex_sel);
    }
}

}  // namespace

void mc_ui_extruder_open() {
    g_ex_sel           = 7U;
    g_ex_feed_step     = MC_UI_EX_FEED_STEP_DEFAULT_IDX;
    g_ex_step          = 0U;
    g_ex_live_cycle    = 0U;

    g_cached_cur_temp_x10 = -99999;
    g_cached_tgt_temp_x10 = -99999;
    g_cached_badge_code   = 255U;
    g_cached_sensor_ok    = false;

    g_cached_heat_active  = false;
    g_cached_rule_on      = false;
    g_cached_fan_on       = false;

    mc_dbg_334_ex_sel_shown  = 7U;
    mc_dbg_335_ex_enter_cnt  = 0U;
    mc_dbg_336_ex_enter_idx  = 255U;
    mc_dbg_337_ex_force      = 0U;
    mc_dbg_338_ex_feed_shown = g_ex_feed_step;
    mc_dbg_339_ex_fake_rule  = 0U;

    (void)mc_encoder_take_clicks();
    (void)mc_encoder_take_press();
}

ExtruderResult mc_ui_extruder_poll() {
    (void)mc_dbg_334_ex_sel_shown;
    (void)mc_dbg_335_ex_enter_cnt;
    (void)mc_dbg_336_ex_enter_idx;
    (void)mc_dbg_337_ex_force;
    (void)mc_dbg_338_ex_feed_shown;
    (void)mc_dbg_339_ex_fake_rule;

    if (mc_dbg_337_ex_force != 0U) {
        const uint32_t force_val = mc_dbg_337_ex_force;
        mc_dbg_337_ex_force = 0U;

        if (force_val == 2U) {
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return ExtruderResult::Back;
        } else if ((force_val >= 3U) && (force_val <= 10U)) {
            const uint8_t target_sel = static_cast<uint8_t>(force_val - 3U);
            if (target_sel != g_ex_sel) {
                apply_ex_selection(target_sel);
            }
        }
    }

    if (g_ex_step < 255U) {
        switch (g_ex_step) {
            case 0:
                mc_display_fill_screen(kScreenBg);
                g_ex_step = 1U;
                return ExtruderResult::Stay;

            case 1: {
                mc_gfx_fill_rect(0U, 0U, 480U, 36U, kTopBarBg);
                mc_gfx_fill_rect(0U, 36U, 480U, 2U, kAccentLine);
                const uint16_t tw = mc_gfx_text_width("EXTRUDER", 2U);
                const uint16_t tx = static_cast<uint16_t>((480U - tw) / 2U);
                mc_gfx_text(tx, 10U, "EXTRUDER", kColorWhite, kTopBarBg, 2U);
                g_ex_step = 2U;
                return ExtruderResult::Stay;
            }

            case 2:
                draw_temp_group();
                g_ex_step = 3U;
                return ExtruderResult::Stay;

            case 3:
                mc_gfx_text(20U, 127U, "HEATER", kLabelGray, kScreenBg, 2U);
                draw_ex_item(0U);
                draw_ex_item(1U);
                g_ex_step = 4U;
                return ExtruderResult::Stay;

            case 4:
                mc_gfx_text(20U, 168U, "FEED", kLabelGray, kScreenBg, 2U);
                mc_gfx_text(20U, 187U, "- RETRACT + EXTRUDE", kLabelGray, kScreenBg, 1U);
                draw_ex_item(2U);
                draw_ex_item(3U);
                draw_ex_item(4U);
                g_ex_step = 5U;
                return ExtruderResult::Stay;

            case 5:
                draw_ex_item(5U);
                draw_ex_item(6U);
                g_ex_step = 6U;
                return ExtruderResult::Stay;

            case 6:
                draw_ex_item(7U);
                g_ex_step = 255U;
                return ExtruderResult::Stay;

            default:
                g_ex_step = 255U;
                return ExtruderResult::Stay;
        }
    }

    const int32_t clicks = mc_encoder_take_clicks();
    if (clicks != 0) {
        int32_t new_idx = static_cast<int32_t>(g_ex_sel) + clicks;
        while (new_idx < 0) {
            new_idx += 8;
        }
        while (new_idx >= 8) {
            new_idx %= 8;
        }
        const uint8_t target_sel = static_cast<uint8_t>(new_idx);
        if (target_sel != g_ex_sel) {
            apply_ex_selection(target_sel);
        }
    }

    if (mc_encoder_take_press()) {
        if (g_ex_sel == 7U) { // BACK
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return ExtruderResult::Back;
        } else if (g_ex_sel == 4U) { // Chips
            g_ex_feed_step = static_cast<uint8_t>((g_ex_feed_step + 1U) % 3U);
            mc_dbg_338_ex_feed_shown = g_ex_feed_step;
            draw_ex_item(4U);
        } else {
            mc_dbg_335_ex_enter_cnt = mc_dbg_335_ex_enter_cnt + 1U;
            mc_dbg_336_ex_enter_idx = g_ex_sel;
        }
    }

    const uint8_t cycle = g_ex_live_cycle;
    g_ex_live_cycle = static_cast<uint8_t>((g_ex_live_cycle + 1U) % 4U);

    if (cycle == 0U) { // Temp Group
        int32_t cur_temp_x10 = 0;
        bool sensor_ok = false;
        const bool fake = (mc_dbg_297_ui_fake_on != 0U);
        if (fake) {
            cur_temp_x10 = mc_dbg_298_ui_fake_noz_x10;
            sensor_ok = (cur_temp_x10 != -9999);
        } else {
            sensor_ok = mc_temp_get_x10(AdcChannel::TempExtruder, &cur_temp_x10);
        }

        int32_t tgt_temp_x10 = 0;
        if (fake) {
            tgt_temp_x10 = mc_dbg_300_ui_fake_noz_tgt_x10;
        } else {
            tgt_temp_x10 = mc_heater_ex_target_x10();
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
    } else if (cycle == 1U) { // HEAT button
        int32_t tgt_temp_x10 = 0;
        if (mc_dbg_297_ui_fake_on != 0U) {
            tgt_temp_x10 = mc_dbg_300_ui_fake_noz_tgt_x10;
        } else {
            tgt_temp_x10 = mc_heater_ex_target_x10();
        }
        const bool is_active = (tgt_temp_x10 == (MC_UI_EX_HEAT_TEMP_C * 10));
        if (is_active != g_cached_heat_active) {
            draw_ex_item(0U);
        }
    } else if (cycle == 2U) { // HOT RULE card
        bool rule_on = false;
        if (mc_dbg_339_ex_fake_rule == 1U) {
            rule_on = true;
        } else if (mc_dbg_339_ex_fake_rule == 2U) {
            rule_on = false;
        } else {
            rule_on = !mc_extruder_cold_ok_get();
        }
        if (rule_on != g_cached_rule_on) {
            draw_ex_item(5U);
        }
    } else if (cycle == 3U) { // FAN card
        bool fan_on = false;
        if (mc_dbg_297_ui_fake_on != 0U) {
            fan_on = (mc_dbg_304_ui_fake_fan_pct > 0U);
        } else {
            fan_on = (mc_fan_get() > 0U);
        }
        if (fan_on != g_cached_fan_on) {
            draw_ex_item(6U);
        }
    }

    return ExtruderResult::Stay;
}

}  // namespace mc
