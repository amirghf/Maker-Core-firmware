// صفحه حرکت رابط کاربری (بند ۸.۸).
// هیچ هدر ST اینجا include نمی‌شود. فقط از hal و gfx و encoder و machine_state استفاده می‌کند.
#include "ui_move.h"
#include "ui_home.h"
#include "ui_colors.h"
#include "../core/gfx.h"
#include "../core/text_format.h"
#include "../core/machine_state.h"
#include "../core/encoder.h"
#include "../hal/hal_display.h"
#include "../hal/hal_stepper.h"

extern "C" {
volatile uint32_t mc_dbg_329_move_sel_shown  = 12;  // خواندنی: اندیس مورد فوکوس‌شده (۰ تا ۱۲)
volatile uint32_t mc_dbg_330_move_enter_cnt  = 0;   // خواندنی: تعداد اینترها روی مورد غیر BACK و غیر STEP
volatile uint32_t mc_dbg_331_move_enter_idx  = 255; // خواندنی: اندیس مورد آخرین اینتر (۲۵۵ = هنوز نشده)
volatile uint32_t mc_dbg_332_move_force      = 0;   // نوشتنی: ۰ = بی‌اثر؛ ۱ = باز کردن صفحه حرکت؛ ۲ = برگشت به منو؛ ۳ تا ۱۵ = فوکوس روی مورد (مقدار - ۳)
volatile uint32_t mc_dbg_333_move_step_shown = 1;   // خواندنی: گام نمایشی: ۰ = ۰.۱، ۱ = ۱ (پیش‌فرض)، ۲ = ۱۰
}

namespace mc {
namespace {

constexpr uint16_t kSelBg         = mc_rgb565(24, 38, 64);
constexpr uint16_t kAxisX         = mc_rgb565(240, 90, 90);
constexpr uint16_t kAxisY         = mc_rgb565(90, 200, 110);
constexpr uint16_t kAxisZ         = mc_rgb565(90, 150, 255);
constexpr uint16_t kAxisOff       = mc_rgb565(70, 80, 100);
constexpr uint16_t kWarn          = mc_rgb565(255, 170, 0);
constexpr uint16_t kWarnFocusBg   = mc_rgb565(60, 40, 8);

static uint8_t g_move_sel        = 12U; // فوکوس اولیه روی BACK (اندیس ۱۲)
static uint8_t g_move_step_idx   = 1U;  // گام نمایشی پیش‌فرض (۱ = 1 mm)
static uint8_t g_move_step       = 0U;  // گام نقاشی ورود (۰ تا ۶، ۲۵۵ = کار عادی)
static uint8_t g_move_axis_cycle = 0U;  // چرخه‌گیری به‌روزرسانی ۳ محور در حالت عادی (۰ تا ۲)

static int32_t g_cached_pos_x10[3] = { -99999, -99999, -99999 };
static bool    g_cached_axis_en[3]  = { false, false, false };

static bool is_move_axis_enabled(uint8_t axis_idx) {
    const bool fake = (mc_dbg_297_ui_fake_on != 0U);
    if (fake) {
        return (mc_dbg_305_ui_fake_axis_en & (1U << axis_idx)) != 0U;
    }
    return mc_stepper_is_enabled(static_cast<StepAxis>(axis_idx));
}

static void draw_axis_row_info(uint8_t r) {
    if (r >= 3U) { return; }
    const uint16_t ry = static_cast<uint16_t>(46U + r * 48U);
    const bool enabled = is_move_axis_enabled(r);

    static const char* const kAxisLetters[3] = { "X", "Y", "Z" };
    static const uint16_t kAxisColors[3]     = { kAxisX, kAxisY, kAxisZ };
    const uint16_t letter_color = enabled ? kAxisColors[r] : kAxisOff;
    mc_gfx_text(20U, static_cast<uint16_t>(ry + 10U), kAxisLetters[r], letter_color, kScreenBg, 3U);

    mc_gfx_fill_rect(60U, static_cast<uint16_t>(ry + 8U), 120U, 16U, kScreenBg);

    const float pos_val = mc_position_get(r);
    char pos_str[16];
    mc_fmt_float(pos_str, sizeof(pos_str), 0U, pos_val, 1U);
    mc_gfx_text(60U, static_cast<uint16_t>(ry + 8U), pos_str, kColorWhite, kScreenBg, 2U);

    mc_gfx_text(60U, static_cast<uint16_t>(ry + 28U), "mm", kLabelGray, kScreenBg, 1U);

    mc_gfx_hline(14U, static_cast<uint16_t>(ry + 46U), 452U, kDividerGray);

    g_cached_pos_x10[r] = static_cast<int32_t>(pos_val >= 0.0f ? (pos_val * 10.0f + 0.5f) : (pos_val * 10.0f - 0.5f));
    g_cached_axis_en[r] = enabled;
}

static void draw_move_item(uint8_t item_idx) {
    if (item_idx >= 13U) { return; }
    const bool focused = (item_idx == g_move_sel);

    if (item_idx <= 8U) {
        const uint8_t r = item_idx / 3U;
        const uint8_t btn_type = item_idx % 3U;
        const uint16_t ry = static_cast<uint16_t>(46U + r * 48U);
        const uint16_t by = static_cast<uint16_t>(ry + 6U);

        uint16_t bx = 190U;
        uint16_t bw = 72U;
        if (btn_type == 1U) {
            bx = 270U;
            bw = 72U;
        } else if (btn_type == 2U) {
            bx = 350U;
            bw = 114U;
        }
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

        if (btn_type == 0U) { // "-"
            const uint16_t cx = static_cast<uint16_t>(bx + (bw - 16U) / 2U);
            const uint16_t cy = static_cast<uint16_t>(by + (bh - 3U) / 2U);
            mc_gfx_fill_rect(cx, cy, 16U, 3U, fg_col);
        } else if (btn_type == 1U) { // "+"
            const uint16_t cx_h = static_cast<uint16_t>(bx + (bw - 16U) / 2U);
            const uint16_t cy_h = static_cast<uint16_t>(by + (bh - 3U) / 2U);
            mc_gfx_fill_rect(cx_h, cy_h, 16U, 3U, fg_col);

            const uint16_t cx_v = static_cast<uint16_t>(bx + (bw - 3U) / 2U);
            const uint16_t cy_v = static_cast<uint16_t>(by + (bh - 16U) / 2U);
            mc_gfx_fill_rect(cx_v, cy_v, 3U, 16U, fg_col);
        } else { // "HOME"
            const uint16_t tw = mc_gfx_text_width("HOME", 2U);
            const uint16_t tx = static_cast<uint16_t>(bx + (bw - tw) / 2U);
            const uint16_t ty = static_cast<uint16_t>(by + (bh - 16U) / 2U);
            mc_gfx_text(tx, ty, "HOME", fg_col, bg_col, 2U);
        }
    } else if (item_idx == 9U) { // STEP
        const uint16_t bg_row = focused ? kSelBg : kScreenBg;
        mc_gfx_fill_rect(8U, 194U, 464U, 36U, bg_row);

        if (focused) {
            mc_gfx_fill_rect(8U, 194U, 4U, 36U, kAccentLine);
        }

        const uint16_t label_col = focused ? kColorWhite : kLabelGray;
        mc_gfx_text(20U, 204U, "STEP", label_col, bg_row, 2U);
        mc_gfx_text(430U, 206U, "mm", kLabelGray, bg_row, 1U);

        static const char* const kChipTexts[3] = { "0.1", "1", "10" };
        static const uint16_t kChipX[3]        = { 190U, 270U, 350U };

        for (uint8_t c = 0U; c < 3U; ++c) {
            const bool is_active = (c == g_move_step_idx);
            const uint16_t cx = kChipX[c];
            constexpr uint16_t cy = 198U;
            constexpr uint16_t cw = 72U;
            constexpr uint16_t ch = 28U;

            if (is_active) {
                mc_gfx_fill_rect(cx, cy, cw, ch, kAccentLine);
                const uint16_t tw = mc_gfx_text_width(kChipTexts[c], 2U);
                const uint16_t tx = static_cast<uint16_t>(cx + (cw - tw) / 2U);
                const uint16_t ty = static_cast<uint16_t>(cy + (ch - 16U) / 2U);
                mc_gfx_text(tx, ty, kChipTexts[c], kScreenBg, kAccentLine, 2U);
            } else {
                mc_gfx_fill_rect(cx, cy, cw, ch, kCardBg);
                mc_gfx_rect(cx, cy, cw, ch, kCardBorder);
                const uint16_t tw = mc_gfx_text_width(kChipTexts[c], 2U);
                const uint16_t tx = static_cast<uint16_t>(cx + (cw - tw) / 2U);
                const uint16_t ty = static_cast<uint16_t>(cy + (ch - 16U) / 2U);
                mc_gfx_text(tx, ty, kChipTexts[c], kLabelGray, kCardBg, 2U);
            }
        }
    } else if (item_idx == 10U) { // HOME ALL
        constexpr uint16_t bx = 8U;
        constexpr uint16_t by = 244U;
        constexpr uint16_t bw = 150U;
        constexpr uint16_t bh = 52U;

        const uint16_t bg_col     = focused ? kSelBg : kCardBg;
        const uint16_t fg_col     = focused ? kColorWhite : kLabelGray;
        const uint16_t border_col = focused ? kAccentLine : kCardBorder;

        mc_gfx_fill_rect(bx, by, bw, bh, bg_col);
        mc_gfx_rect(bx, by, bw, bh, border_col);
        if (focused) {
            mc_gfx_rect(static_cast<uint16_t>(bx + 1U), static_cast<uint16_t>(by + 1U),
                        static_cast<uint16_t>(bw - 2U), static_cast<uint16_t>(bh - 2U), border_col);
        }

        const uint16_t tw = mc_gfx_text_width("HOME ALL", 2U);
        const uint16_t tx = static_cast<uint16_t>(bx + (bw - tw) / 2U);
        const uint16_t ty = static_cast<uint16_t>(by + (bh - 16U) / 2U);
        mc_gfx_text(tx, ty, "HOME ALL", fg_col, bg_col, 2U);
    } else if (item_idx == 11U) { // MOTORS OFF
        constexpr uint16_t bx = 166U;
        constexpr uint16_t by = 244U;
        constexpr uint16_t bw = 150U;
        constexpr uint16_t bh = 52U;

        const uint16_t bg_col     = focused ? kWarnFocusBg : kCardBg;
        const uint16_t fg_col     = focused ? kColorWhite : kWarn;
        const uint16_t border_col = kWarn;

        mc_gfx_fill_rect(bx, by, bw, bh, bg_col);
        mc_gfx_rect(bx, by, bw, bh, border_col);
        if (focused) {
            mc_gfx_rect(static_cast<uint16_t>(bx + 1U), static_cast<uint16_t>(by + 1U),
                        static_cast<uint16_t>(bw - 2U), static_cast<uint16_t>(bh - 2U), border_col);
        }

        const uint16_t tw = mc_gfx_text_width("MOTORS OFF", 2U);
        const uint16_t tx = static_cast<uint16_t>(bx + (bw - tw) / 2U);
        const uint16_t ty = static_cast<uint16_t>(by + (bh - 16U) / 2U);
        mc_gfx_text(tx, ty, "MOTORS OFF", fg_col, bg_col, 2U);
    } else if (item_idx == 12U) { // BACK
        constexpr uint16_t bx = 324U;
        constexpr uint16_t by = 244U;
        constexpr uint16_t bw = 148U;
        constexpr uint16_t bh = 52U;

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

static void apply_move_selection(uint8_t target_sel) {
    if (target_sel >= 13U) { return; }
    const uint8_t old_sel = g_move_sel;
    g_move_sel = target_sel;
    mc_dbg_329_move_sel_shown = g_move_sel;

    if (old_sel != g_move_sel) {
        draw_move_item(old_sel);
        draw_move_item(g_move_sel);
    }
}

}  // namespace

void mc_ui_move_open() {
    g_move_sel        = 12U;
    g_move_step_idx   = 1U;
    g_move_step       = 0U;
    g_move_axis_cycle = 0U;

    g_cached_pos_x10[0] = -99999;
    g_cached_pos_x10[1] = -99999;
    g_cached_pos_x10[2] = -99999;
    g_cached_axis_en[0] = false;
    g_cached_axis_en[1] = false;
    g_cached_axis_en[2] = false;

    mc_dbg_329_move_sel_shown  = 12U;
    mc_dbg_333_move_step_shown = 1U;

    (void)mc_encoder_take_clicks();
    (void)mc_encoder_take_press();
}

MoveResult mc_ui_move_poll() {
    (void)mc_dbg_329_move_sel_shown;
    (void)mc_dbg_330_move_enter_cnt;
    (void)mc_dbg_331_move_enter_idx;
    (void)mc_dbg_332_move_force;
    (void)mc_dbg_333_move_step_shown;

    if (mc_dbg_332_move_force != 0U) {
        const uint32_t force_val = mc_dbg_332_move_force;
        mc_dbg_332_move_force = 0U;

        if (force_val == 2U) {
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return MoveResult::Back;
        } else if ((force_val >= 3U) && (force_val <= 15U)) {
            const uint8_t target_sel = static_cast<uint8_t>(force_val - 3U);
            if (target_sel != g_move_sel) {
                apply_move_selection(target_sel);
            }
        }
    }

    if (g_move_step < 255U) {
        switch (g_move_step) {
            case 0:
                mc_display_fill_screen(kScreenBg);
                g_move_step = 1U;
                return MoveResult::Stay;

            case 1: {
                mc_gfx_fill_rect(0U, 0U, 480U, 36U, kTopBarBg);
                mc_gfx_fill_rect(0U, 36U, 480U, 2U, kAccentLine);
                const uint16_t tw = mc_gfx_text_width("MOVE", 2U);
                const uint16_t tx = static_cast<uint16_t>((480U - tw) / 2U);
                mc_gfx_text(tx, 10U, "MOVE", kColorWhite, kTopBarBg, 2U);
                g_move_step = 2U;
                return MoveResult::Stay;
            }

            case 2:
                draw_axis_row_info(0U);
                draw_move_item(0U);
                draw_move_item(1U);
                draw_move_item(2U);
                g_move_step = 3U;
                return MoveResult::Stay;

            case 3:
                draw_axis_row_info(1U);
                draw_move_item(3U);
                draw_move_item(4U);
                draw_move_item(5U);
                g_move_step = 4U;
                return MoveResult::Stay;

            case 4:
                draw_axis_row_info(2U);
                draw_move_item(6U);
                draw_move_item(7U);
                draw_move_item(8U);
                g_move_step = 5U;
                return MoveResult::Stay;

            case 5:
                draw_move_item(9U);
                g_move_step = 6U;
                return MoveResult::Stay;

            case 6:
                draw_move_item(10U);
                draw_move_item(11U);
                draw_move_item(12U);
                g_move_step = 255U;
                return MoveResult::Stay;

            default:
                g_move_step = 255U;
                return MoveResult::Stay;
        }
    }

    const int32_t clicks = mc_encoder_take_clicks();
    if (clicks != 0) {
        int32_t new_idx = static_cast<int32_t>(g_move_sel) + clicks;
        while (new_idx < 0) {
            new_idx += 13;
        }
        while (new_idx >= 13) {
            new_idx %= 13;
        }
        const uint8_t target_sel = static_cast<uint8_t>(new_idx);
        if (target_sel != g_move_sel) {
            apply_move_selection(target_sel);
        }
    }

    if (mc_encoder_take_press()) {
        if (g_move_sel == 12U) {
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return MoveResult::Back;
        } else if (g_move_sel == 9U) {
            g_move_step_idx = static_cast<uint8_t>((g_move_step_idx + 1U) % 3U);
            mc_dbg_333_move_step_shown = g_move_step_idx;
            draw_move_item(9U);
        } else {
            mc_dbg_330_move_enter_cnt = mc_dbg_330_move_enter_cnt + 1U;
            mc_dbg_331_move_enter_idx = g_move_sel;
        }
    }

    const uint8_t axis_r = g_move_axis_cycle;
    g_move_axis_cycle = static_cast<uint8_t>((g_move_axis_cycle + 1U) % 3U);

    const bool curr_en = is_move_axis_enabled(axis_r);
    const float curr_pos = mc_position_get(axis_r);
    const int32_t curr_pos_x10 = static_cast<int32_t>(curr_pos >= 0.0f ? (curr_pos * 10.0f + 0.5f) : (curr_pos * 10.0f - 0.5f));

    if ((curr_en != g_cached_axis_en[axis_r]) || (curr_pos_x10 != g_cached_pos_x10[axis_r])) {
        draw_axis_row_info(axis_r);
    }

    return MoveResult::Stay;
}

}  // namespace mc
