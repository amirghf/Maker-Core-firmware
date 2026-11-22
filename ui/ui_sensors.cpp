// صفحه سنسورها رابط کاربری (بند ۸.۱۲).
// هیچ هدر ST اینجا include نمی‌شود. فقط از hal و gfx و encoder و endstop و bltouch و hal_gpio استفاده می‌کند.
#include "ui_sensors.h"
#include "ui_home.h"
#include "ui_colors.h"
#include "../core/gfx.h"
#include "../core/text_format.h"
#include "../core/endstop.h"
#include "../core/bltouch.h"
#include "../core/encoder.h"
#include "../hal/hal_gpio.h"
#include "../hal/hal_display.h"
#include "../config/mc_config.h"

extern "C" {
volatile uint32_t mc_dbg_344_sen_sel_shown  = 4;   // خواندنی: اندیس مورد فوکوس‌شده (۰ تا ۴)
volatile uint32_t mc_dbg_345_sen_enter_cnt  = 0;   // خواندنی: تعداد اینترها روی مورد غیر BACK
volatile uint32_t mc_dbg_346_sen_enter_idx  = 255; // خواندنی: اندیس مورد آخرین اینتر (۲۵۵ = هنوز نشده)
volatile uint32_t mc_dbg_347_sen_force      = 0;   // نوشتنی: ۰ = بی‌اثر؛ ۱ = باز کردن صفحه سنسورها؛ ۲ = برگشت به منو؛ ۳ تا ۷ = فوکوس روی مورد (مقدار - ۳)
volatile uint32_t mc_dbg_348_sen_fake       = 0;   // نوشتنی: حالت ساختگی ظاهر صفحه سنسورها (بیت ۷ = فعال‌سازی)
}

namespace mc {
namespace {

constexpr uint16_t kSelBg         = mc_rgb565(24, 38, 64);
constexpr uint16_t kAxisX         = mc_rgb565(240, 90, 90);
constexpr uint16_t kAxisY         = mc_rgb565(90, 200, 110);
constexpr uint16_t kAxisZ         = mc_rgb565(90, 150, 255);
constexpr uint16_t kWarn          = mc_rgb565(255, 170, 0);
constexpr uint16_t kWarnFocusBg   = mc_rgb565(60, 40, 8);

static uint8_t g_sen_sel        = 4U; // فوکوس اولیه روی BACK (اندیس ۴)
static uint8_t g_sen_step       = 0U; // گام نقاشی ورود (۰ تا ۵، ۲۵۵ = کار عادی)
static uint8_t g_sen_live_cycle = 0U; // چرخه‌گیری به‌روزرسانی ۵ بخش در حالت عادی (۰ تا ۴)

static bool       g_cached_endstop_trig[3] = { false, false, false };
static bool       g_cached_bltouch_busy    = false;
static uint8_t    g_cached_bltouch_pin     = 255U;
static bool       g_cached_fil_empty       = false;

static void draw_dot(uint16_t cx, uint16_t cy, uint16_t color) {
    mc_gfx_fill_rect(static_cast<uint16_t>(cx - 2U), static_cast<uint16_t>(cy - 4U), 5U, 9U, color);
    mc_gfx_fill_rect(static_cast<uint16_t>(cx - 3U), static_cast<uint16_t>(cy - 3U), 7U, 7U, color);
    mc_gfx_fill_rect(static_cast<uint16_t>(cx - 4U), static_cast<uint16_t>(cy - 2U), 9U, 5U, color);
}

static bool eval_endstop_trig(uint8_t axis) {
    if ((mc_dbg_348_sen_fake & 0x80U) != 0U) {
        return (mc_dbg_348_sen_fake & (1U << axis)) != 0U;
    }
    return mc_endstop_triggered(axis);
}

static bool eval_bltouch_busy() {
    if ((mc_dbg_348_sen_fake & 0x80U) != 0U) {
        return (mc_dbg_348_sen_fake & 0x40U) != 0U;
    }
    return mc_bltouch_busy();
}

static uint8_t eval_bltouch_pin_code() { // 0=Unknown, 1=Deployed, 2=Stowed
    if ((mc_dbg_348_sen_fake & 0x80U) != 0U) {
        return static_cast<uint8_t>((mc_dbg_348_sen_fake >> 4U) & 0x03U);
    }
    const BltouchPin pin_st = mc_bltouch_pin_state();
    return static_cast<uint8_t>(pin_st);
}

static bool eval_fil_empty() {
    if ((mc_dbg_348_sen_fake & 0x80U) != 0U) {
        return (mc_dbg_348_sen_fake & 0x08U) != 0U;
    }
    const bool pin_high = mc_gpio_read(PinId::FilamentSensor);
    return (MC_UI_FIL_EMPTY_LEVEL != 0) ? pin_high : !pin_high;
}

static void draw_endstop_card(uint8_t axis) {
    if (axis >= 3U) { return; }
    static const uint16_t kCardX[3] = { 8U, 166U, 324U };
    static const char* const kAxisLabels[3] = { "X", "Y", "Z" };
    static const uint16_t kAxisColors[3] = { kAxisX, kAxisY, kAxisZ };

    const uint16_t cx = kCardX[axis];
    constexpr uint16_t cy = 46U;
    constexpr uint16_t cw = 148U;
    constexpr uint16_t ch = 44U;

    mc_gfx_fill_rect(cx, cy, cw, ch, kCardBg);
    mc_gfx_rect(cx, cy, cw, ch, kCardBorder);

    mc_gfx_text(static_cast<uint16_t>(cx + 12U), 58U, kAxisLabels[axis], kAxisColors[axis], kCardBg, 3U);

    const bool trig = eval_endstop_trig(axis);
    const char* st_str = trig ? "HIT" : "OK";
    const uint16_t st_color = trig ? kColorRed : kColorGreen;

    const uint16_t tw = mc_gfx_text_width(st_str, 2U);
    const uint16_t tx = static_cast<uint16_t>(cx + cw - 10U - tw);
    mc_gfx_text(tx, 61U, st_str, st_color, kCardBg, 2U);

    g_cached_endstop_trig[axis] = trig;
}

static void draw_bltouch_badge() {
    const bool busy = eval_bltouch_busy();
    const uint8_t pin_code = eval_bltouch_pin_code();

    const char* st_str = "UNKNOWN";
    uint16_t badge_bg = kCardBg;
    uint16_t badge_fg = kLabelGray;

    if (busy) {
        st_str = "BUSY";
        badge_bg = kYellowBusy;
        badge_fg = kScreenBg;
    } else if (pin_code == 1U) {
        st_str = "DEPLOYED";
        badge_bg = kYellowBusy;
        badge_fg = kScreenBg;
    } else if (pin_code == 2U) {
        st_str = "STOWED";
        badge_bg = kColorGreen;
        badge_fg = kScreenBg;
    } else {
        st_str = "UNKNOWN";
        badge_bg = kCardBg;
        badge_fg = kLabelGray;
    }

    mc_gfx_fill_rect(352U, 100U, 112U, 22U, badge_bg);
    const uint16_t tw = mc_gfx_text_width(st_str, 2U);
    const uint16_t tx = static_cast<uint16_t>(352U + (112U - tw) / 2U);
    constexpr uint16_t ty = 103U;
    mc_gfx_text(tx, ty, st_str, badge_fg, badge_bg, 2U);

    g_cached_bltouch_busy = busy;
    g_cached_bltouch_pin  = pin_code;
}

static void draw_filament_card() {
    constexpr uint16_t bx = 8U;
    constexpr uint16_t by = 182U;
    constexpr uint16_t bw = 464U;
    constexpr uint16_t bh = 40U;

    mc_gfx_fill_rect(bx, by, bw, bh, kCardBg);
    mc_gfx_rect(bx, by, bw, bh, kCardBorder);

    mc_gfx_text(22U, 195U, "FILAMENT", kLabelGray, kCardBg, 2U);

    const bool empty = eval_fil_empty();
    const char* st_str = empty ? "EMPTY" : "OK";
    const uint16_t st_color = empty ? kColorRed : kColorGreen;

    const uint16_t tw = mc_gfx_text_width(st_str, 2U);
    const uint16_t tx = static_cast<uint16_t>(462U - tw);
    mc_gfx_text(tx, 195U, st_str, st_color, kCardBg, 2U);

    const uint16_t dot_cx = static_cast<uint16_t>(tx - 16U);
    draw_dot(dot_cx, 202U, st_color);

    g_cached_fil_empty = empty;
}

static void draw_sensor_item(uint8_t item_idx) {
    if (item_idx >= 5U) { return; }
    const bool focused = (item_idx == g_sen_sel);

    if (item_idx <= 3U) {
        static const uint16_t kBtnX[4] = { 8U, 126U, 244U, 362U };
        static const char* const kBtnLabels[4] = { "DEPLOY", "STOW", "TEST", "ALM CLR" };

        const uint16_t bx = kBtnX[item_idx];
        constexpr uint16_t by = 128U;
        constexpr uint16_t bw = 110U;
        constexpr uint16_t bh = 40U;

        uint16_t bg_col     = focused ? kSelBg : kCardBg;
        uint16_t border_col = focused ? kAccentLine : kCardBorder;
        uint16_t fg_col     = focused ? kColorWhite : kLabelGray;

        if (item_idx == 3U) { // ALM CLR
            bg_col     = focused ? kWarnFocusBg : kCardBg;
            border_col = kWarn;
            fg_col     = focused ? kColorWhite : kWarn;
        }

        mc_gfx_fill_rect(bx, by, bw, bh, bg_col);
        mc_gfx_rect(bx, by, bw, bh, border_col);
        if (focused) {
            mc_gfx_rect(static_cast<uint16_t>(bx + 1U), static_cast<uint16_t>(by + 1U),
                        static_cast<uint16_t>(bw - 2U), static_cast<uint16_t>(bh - 2U), border_col);
        }

        const uint16_t tw = mc_gfx_text_width(kBtnLabels[item_idx], 2U);
        const uint16_t tx = static_cast<uint16_t>(bx + (bw - tw) / 2U);
        const uint16_t ty = static_cast<uint16_t>(by + (bh - 16U) / 2U);
        mc_gfx_text(tx, ty, kBtnLabels[item_idx], fg_col, bg_col, 2U);
    } else if (item_idx == 4U) { // BACK
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

static void apply_sensors_selection(uint8_t target_sel) {
    if (target_sel >= 5U) { return; }
    const uint8_t old_sel = g_sen_sel;
    g_sen_sel = target_sel;
    mc_dbg_344_sen_sel_shown = g_sen_sel;

    if (old_sel != g_sen_sel) {
        draw_sensor_item(old_sel);
        draw_sensor_item(g_sen_sel);
    }
}

}  // namespace

void mc_ui_sensors_open() {
    g_sen_sel        = 4U;
    g_sen_step       = 0U;
    g_sen_live_cycle = 0U;

    g_cached_endstop_trig[0] = false;
    g_cached_endstop_trig[1] = false;
    g_cached_endstop_trig[2] = false;
    g_cached_bltouch_busy    = false;
    g_cached_bltouch_pin     = 255U;
    g_cached_fil_empty       = false;

    mc_dbg_344_sen_sel_shown = 4U;
    mc_dbg_345_sen_enter_cnt = 0U;
    mc_dbg_346_sen_enter_idx = 255U;
    mc_dbg_347_sen_force     = 0U;
    mc_dbg_348_sen_fake      = 0U;

    (void)mc_encoder_take_clicks();
    (void)mc_encoder_take_press();
}

SensorsResult mc_ui_sensors_poll() {
    (void)mc_dbg_344_sen_sel_shown;
    (void)mc_dbg_345_sen_enter_cnt;
    (void)mc_dbg_346_sen_enter_idx;
    (void)mc_dbg_347_sen_force;
    (void)mc_dbg_348_sen_fake;

    if (mc_dbg_347_sen_force != 0U) {
        const uint32_t force_val = mc_dbg_347_sen_force;
        mc_dbg_347_sen_force = 0U;

        if (force_val == 2U) {
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return SensorsResult::Back;
        } else if ((force_val >= 3U) && (force_val <= 7U)) {
            const uint8_t target_sel = static_cast<uint8_t>(force_val - 3U);
            if (target_sel != g_sen_sel) {
                apply_sensors_selection(target_sel);
            }
        }
    }

    if (g_sen_step < 255U) {
        switch (g_sen_step) {
            case 0:
                mc_display_fill_screen(kScreenBg);
                g_sen_step = 1U;
                return SensorsResult::Stay;

            case 1: {
                mc_gfx_fill_rect(0U, 0U, 480U, 36U, kTopBarBg);
                mc_gfx_fill_rect(0U, 36U, 480U, 2U, kAccentLine);
                const uint16_t tw = mc_gfx_text_width("SENSORS", 2U);
                const uint16_t tx = static_cast<uint16_t>((480U - tw) / 2U);
                mc_gfx_text(tx, 10U, "SENSORS", kColorWhite, kTopBarBg, 2U);
                g_sen_step = 2U;
                return SensorsResult::Stay;
            }

            case 2:
                draw_endstop_card(0U);
                draw_endstop_card(1U);
                draw_endstop_card(2U);
                g_sen_step = 3U;
                return SensorsResult::Stay;

            case 3:
                mc_gfx_text(20U, 104U, "BLTOUCH", kLabelGray, kScreenBg, 2U);
                draw_bltouch_badge();
                draw_sensor_item(0U);
                draw_sensor_item(1U);
                draw_sensor_item(2U);
                draw_sensor_item(3U);
                g_sen_step = 4U;
                return SensorsResult::Stay;

            case 4:
                draw_filament_card();
                g_sen_step = 5U;
                return SensorsResult::Stay;

            case 5:
                draw_sensor_item(4U);
                g_sen_step = 255U;
                return SensorsResult::Stay;

            default:
                g_sen_step = 255U;
                return SensorsResult::Stay;
        }
    }

    const int32_t clicks = mc_encoder_take_clicks();
    if (clicks != 0) {
        int32_t new_idx = static_cast<int32_t>(g_sen_sel) + clicks;
        while (new_idx < 0) {
            new_idx += 5;
        }
        while (new_idx >= 5) {
            new_idx %= 5;
        }
        const uint8_t target_sel = static_cast<uint8_t>(new_idx);
        if (target_sel != g_sen_sel) {
            apply_sensors_selection(target_sel);
        }
    }

    if (mc_encoder_take_press()) {
        if (g_sen_sel == 4U) { // BACK
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return SensorsResult::Back;
        } else {
            mc_dbg_345_sen_enter_cnt = mc_dbg_345_sen_enter_cnt + 1U;
            mc_dbg_346_sen_enter_idx = g_sen_sel;
        }
    }

    const uint8_t cycle = g_sen_live_cycle;
    g_sen_live_cycle = static_cast<uint8_t>((g_sen_live_cycle + 1U) % 5U);

    if (cycle == 0U) { // Endstop X
        const bool trig = eval_endstop_trig(0U);
        if (trig != g_cached_endstop_trig[0]) {
            draw_endstop_card(0U);
        }
    } else if (cycle == 1U) { // Endstop Y
        const bool trig = eval_endstop_trig(1U);
        if (trig != g_cached_endstop_trig[1]) {
            draw_endstop_card(1U);
        }
    } else if (cycle == 2U) { // Endstop Z
        const bool trig = eval_endstop_trig(2U);
        if (trig != g_cached_endstop_trig[2]) {
            draw_endstop_card(2U);
        }
    } else if (cycle == 3U) { // BLTouch status badge
        const bool busy = eval_bltouch_busy();
        const uint8_t pin_code = eval_bltouch_pin_code();
        if ((busy != g_cached_bltouch_busy) || (pin_code != g_cached_bltouch_pin)) {
            draw_bltouch_badge();
        }
    } else if (cycle == 4U) { // Filament card
        const bool empty = eval_fil_empty();
        if (empty != g_cached_fil_empty) {
            draw_filament_card();
        }
    }

    return SensorsResult::Stay;
}

}  // namespace mc
