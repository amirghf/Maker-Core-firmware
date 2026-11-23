// صفحه SD CARD رابط کاربری (بند ۸.۱۴).
// هیچ هدر ST اینجا include نمی‌شود. فقط از hal و gfx و encoder استفاده می‌کند.
#include "ui_sd.h"
#include "ui_home.h"
#include "ui_colors.h"
#include "../core/gfx.h"
#include "../core/text_format.h"
#include "../core/encoder.h"
#include "../hal/hal_display.h"
#include "../config/mc_config.h"

extern "C" {
volatile uint32_t mc_dbg_349_sd_sel_shown = 2;   // خواندنی: اندیس مورد فوکوس‌شده (۰ تا ۲)
volatile uint32_t mc_dbg_350_sd_enter_cnt = 0;   // خواندنی: تعداد اینترها روی مورد غیر BACK
volatile uint32_t mc_dbg_351_sd_enter_idx = 255; // خواندنی: اندیس مورد آخرین اینتر (۲۵۵ = هنوز نشده)
volatile uint32_t mc_dbg_352_sd_force     = 0;   // نوشتنی: ۰ = بی‌اثر؛ ۱ = باز کردن صفحه؛ ۲ = برگشت به منو؛ ۳ تا ۵ = فوکوس روی مورد (مقدار - ۳)
volatile uint32_t mc_dbg_353_sd_fake      = 0;   // نوشتنی: حالت ساختگی ظاهر صفحه SD CARD (بیت ۷ = فعال‌سازی)
}

namespace mc {
namespace {

constexpr uint16_t kSelBg    = mc_rgb565(24, 38, 64);
constexpr uint16_t kWarn     = mc_rgb565(255, 170, 0);
constexpr uint16_t kAccMedia = mc_rgb565(0, 200, 100); // رنگ شاخص SD CARD در منو

static uint8_t g_sd_sel        = 2U; // فوکوس اولیه روی BACK (اندیس ۲)
static uint8_t g_sd_step       = 0U; // گام نقاشی ورود (۰ تا ۵، ۲۵۵ = کار عادی)
static uint8_t g_sd_live_cycle = 0U; // چرخه‌گیری به‌روزرسانی زنده (۰ تا ۱)
static uint8_t g_cached_status_code = 255U;

// در بند ۱۱ درایور واقعی SD وصل می‌شود
static bool real_sd_present() {
    return false;
}

// 0 = وجود ندارد، 1 = آماده، 2 = خطا، 3 = مشغول
static uint8_t read_sd_status_code() {
    const uint32_t fake = mc_dbg_353_sd_fake;
    if ((fake & 0x80U) != 0U) { // بیت ۷ = فعال بودن fake
        const bool present = (fake & 0x01U) != 0U;
        const bool error   = (fake & 0x02U) != 0U;
        const bool busy    = (fake & 0x04U) != 0U;

        if (!present) { return 0U; }
        if (error)   { return 2U; }
        if (busy)    { return 3U; }
        return 1U;
    }
    return real_sd_present() ? 1U : 0U;
}

// ۱. کارت وضعیت (CAPACITY و badge)
static void draw_status_card() {
    constexpr uint16_t bx = 8U;
    constexpr uint16_t by = 46U;
    constexpr uint16_t bw = 464U;
    constexpr uint16_t bh = 56U;

    mc_gfx_fill_rect(bx, by, bw, bh, kCardBg);
    mc_gfx_rect(bx, by, bw, bh, kCardBorder);

    // نوار رنگی سمت چپ
    mc_gfx_fill_rect(bx, by, 4U, bh, kAccMedia);

    mc_gfx_text(22U, 54U, "CAPACITY", kLabelGray, kCardBg, 1U);

    const uint8_t st_code = read_sd_status_code();
    const bool fake_on = ((mc_dbg_353_sd_fake & 0x80U) != 0U);
    const bool present = fake_on && ((mc_dbg_353_sd_fake & 0x01U) != 0U);

    const char* cap_str = present ? "7.4 GB" : "---";
    mc_gfx_fill_rect(22U, 70U, 150U, 16U, kCardBg);
    mc_gfx_text(22U, 70U, cap_str, kColorWhite, kCardBg, 2U);

    // نشان وضعیت (badge)
    const char* badge_text = "NO CARD";
    uint16_t badge_bg = kCardBg;
    uint16_t badge_fg = kLabelGray;

    if (st_code == 1U) {
        badge_text = "READY";
        badge_bg   = kColorGreen;
        badge_fg   = kScreenBg;
    } else if (st_code == 2U) {
        badge_text = "ERROR";
        badge_bg   = kColorRed;
        badge_fg   = kColorWhite;
    } else if (st_code == 3U) {
        badge_text = "BUSY";
        badge_bg   = kYellowBusy;
        badge_fg   = kScreenBg;
    } else {
        badge_text = "NO CARD";
        badge_bg   = kCardBg;
        badge_fg   = kLabelGray;
    }

    constexpr uint16_t badge_x = 352U;
    constexpr uint16_t badge_y = 61U;
    constexpr uint16_t badge_w = 112U;
    constexpr uint16_t badge_h = 26U;

    mc_gfx_fill_rect(badge_x, badge_y, badge_w, badge_h, badge_bg);

    const uint16_t tw = mc_gfx_text_width(badge_text, 2U);
    const uint16_t tx = static_cast<uint16_t>(badge_x + (badge_w - tw) / 2U);
    constexpr uint16_t ty = 66U;
    mc_gfx_text(tx, ty, badge_text, badge_fg, badge_bg, 2U);

    g_cached_status_code = st_code;
}

// ۲. ناحیه فهرست فایل‌ها و متون پایین
static void draw_file_list_area() {
    static const uint16_t kRowY[4] = { 112U, 146U, 180U, 214U };
    static const char* kSampleFiles[4] = {
        "BENCHY.GCO",
        "CALIBRATION.GCO",
        "BRACKET_V2.GCO",
        "TEST_CUBE.GCO"
    };

    const uint8_t st_code = read_sd_status_code();

    for (uint8_t r = 0U; r < 4U; ++r) {
        const uint16_t ry = kRowY[r];
        mc_gfx_fill_rect(8U, ry, 380U, 32U, kCardBg);
        mc_gfx_rect(8U, ry, 380U, 32U, kCardBorder);

        if (st_code == 0U) { // وجود ندارد
            if (r == 0U) {
                mc_gfx_text(22U, static_cast<uint16_t>(ry + 8U), "INSERT SD CARD", kLabelGray, kCardBg, 2U);
            }
        } else if (st_code == 2U) { // خطا
            if (r == 0U) {
                mc_gfx_text(22U, static_cast<uint16_t>(ry + 8U), "READ ERROR", kColorRed, kCardBg, 2U);
            }
        } else { // آماده یا مشغول
            mc_gfx_text(22U, static_cast<uint16_t>(ry + 8U), kSampleFiles[r], kColorWhite, kCardBg, 2U);
        }
    }

    // متن پایین FILES
    mc_gfx_fill_rect(20U, 270U, 200U, 16U, kScreenBg);
    mc_gfx_text(20U, 270U, "FILES", kLabelGray, kScreenBg, 2U);

    if (st_code == 1U) { // آماده
        mc_gfx_text(92U, 270U, "4", kColorWhite, kScreenBg, 2U);
    } else {
        mc_gfx_text(92U, 270U, "---", kLabelGray, kScreenBg, 2U);
    }
}

// ۳. دکمه‌های قابل فوکوس (UP, DOWN, BACK)
static void draw_sd_item(uint8_t item_idx) {
    if (item_idx >= 3U) { return; }
    const bool focused = (item_idx == g_sd_sel);

    if (item_idx == 0U) { // UP Button
        constexpr uint16_t bx = 396U;
        constexpr uint16_t by = 112U;
        constexpr uint16_t bw = 76U;
        constexpr uint16_t bh = 66U;

        const uint16_t bg_col     = focused ? kSelBg : kCardBg;
        const uint16_t border_col = focused ? kAccentLine : kCardBorder;
        const uint16_t tri_col    = focused ? kColorWhite : kLabelGray;

        mc_gfx_fill_rect(bx, by, bw, bh, bg_col);
        mc_gfx_rect(bx, by, bw, bh, border_col);
        if (focused) {
            mc_gfx_rect(static_cast<uint16_t>(bx + 1U), static_cast<uint16_t>(by + 1U),
                        static_cast<uint16_t>(bw - 2U), static_cast<uint16_t>(bh - 2U), border_col);
        }

        // مثلث نوک‌بالا برای UP
        constexpr uint16_t cx = 434U;
        constexpr uint16_t y_top = 140U;
        for (uint16_t i = 0U; i <= 9U; ++i) {
            const uint16_t hx = static_cast<uint16_t>(cx - i);
            const uint16_t hy = static_cast<uint16_t>(y_top + i);
            const uint16_t hw = static_cast<uint16_t>(2U * i + 1U);
            mc_gfx_hline(hx, hy, hw, tri_col);
        }
    } else if (item_idx == 1U) { // DOWN Button
        constexpr uint16_t bx = 396U;
        constexpr uint16_t by = 180U;
        constexpr uint16_t bw = 76U;
        constexpr uint16_t bh = 66U;

        const uint16_t bg_col     = focused ? kSelBg : kCardBg;
        const uint16_t border_col = focused ? kAccentLine : kCardBorder;
        const uint16_t tri_col    = focused ? kColorWhite : kLabelGray;

        mc_gfx_fill_rect(bx, by, bw, bh, bg_col);
        mc_gfx_rect(bx, by, bw, bh, border_col);
        if (focused) {
            mc_gfx_rect(static_cast<uint16_t>(bx + 1U), static_cast<uint16_t>(by + 1U),
                        static_cast<uint16_t>(bw - 2U), static_cast<uint16_t>(bh - 2U), border_col);
        }

        // مثلث نوک‌پایین برای DOWN
        constexpr uint16_t cx = 434U;
        constexpr uint16_t y_top = 208U;
        for (uint16_t i = 0U; i <= 9U; ++i) {
            const uint16_t width_idx = static_cast<uint16_t>(9U - i);
            const uint16_t hx = static_cast<uint16_t>(cx - width_idx);
            const uint16_t hy = static_cast<uint16_t>(y_top + i);
            const uint16_t hw = static_cast<uint16_t>(2U * width_idx + 1U);
            mc_gfx_hline(hx, hy, hw, tri_col);
        }
    } else if (item_idx == 2U) { // BACK Button
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

static void apply_sd_selection(uint8_t target_sel) {
    if (target_sel >= 3U) { return; }
    const uint8_t old_sel = g_sd_sel;
    g_sd_sel = target_sel;
    mc_dbg_349_sd_sel_shown = g_sd_sel;

    if (old_sel != g_sd_sel) {
        draw_sd_item(old_sel);
        draw_sd_item(g_sd_sel);
    }
}

}  // namespace

void mc_ui_sd_open() {
    g_sd_sel             = 2U; // فوکوس اولیه روی BACK (اندیس ۲)
    g_sd_step            = 0U;
    g_sd_live_cycle      = 0U;
    g_cached_status_code = 255U;

    mc_dbg_349_sd_sel_shown = 2U;
    mc_dbg_350_sd_enter_cnt = 0U;
    mc_dbg_351_sd_enter_idx = 255U;
    mc_dbg_352_sd_force     = 0U;
    mc_dbg_353_sd_fake      = 0U;

    (void)mc_encoder_take_clicks();
    (void)mc_encoder_take_press();
}

SdResult mc_ui_sd_poll() {
    (void)mc_dbg_349_sd_sel_shown;
    (void)mc_dbg_350_sd_enter_cnt;
    (void)mc_dbg_351_sd_enter_idx;
    (void)mc_dbg_352_sd_force;
    (void)mc_dbg_353_sd_fake;

    if (mc_dbg_352_sd_force != 0U) {
        const uint32_t force_val = mc_dbg_352_sd_force;
        mc_dbg_352_sd_force = 0U;

        if (force_val == 2U) {
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return SdResult::Back;
        } else if ((force_val >= 3U) && (force_val <= 5U)) {
            const uint8_t target_sel = static_cast<uint8_t>(force_val - 3U);
            if (target_sel != g_sd_sel) {
                apply_sd_selection(target_sel);
            }
        }
    }

    if (g_sd_step < 255U) {
        switch (g_sd_step) {
            case 0:
                mc_display_fill_screen(kScreenBg);
                g_sd_step = 1U;
                return SdResult::Stay;

            case 1: {
                mc_gfx_fill_rect(0U, 0U, 480U, 36U, kTopBarBg);
                mc_gfx_fill_rect(0U, 36U, 480U, 2U, kAccentLine);
                const uint16_t tw = mc_gfx_text_width("SD CARD", 2U);
                const uint16_t tx = static_cast<uint16_t>((480U - tw) / 2U);
                mc_gfx_text(tx, 10U, "SD CARD", kColorWhite, kTopBarBg, 2U);
                g_sd_step = 2U;
                return SdResult::Stay;
            }

            case 2:
                draw_status_card();
                g_sd_step = 3U;
                return SdResult::Stay;

            case 3:
                draw_file_list_area();
                g_sd_step = 4U;
                return SdResult::Stay;

            case 4:
                draw_sd_item(0U);
                draw_sd_item(1U);
                g_sd_step = 5U;
                return SdResult::Stay;

            case 5:
                draw_sd_item(2U);
                g_sd_step = 255U;
                return SdResult::Stay;

            default:
                g_sd_step = 255U;
                return SdResult::Stay;
        }
    }

    const int32_t clicks = mc_encoder_take_clicks();
    if (clicks != 0) {
        int32_t new_idx = static_cast<int32_t>(g_sd_sel) + clicks;
        while (new_idx < 0) {
            new_idx += 3;
        }
        while (new_idx >= 3) {
            new_idx %= 3;
        }
        const uint8_t target_sel = static_cast<uint8_t>(new_idx);
        if (target_sel != g_sd_sel) {
            apply_sd_selection(target_sel);
        }
    }

    if (mc_encoder_take_press()) {
        if (g_sd_sel == 2U) { // BACK
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return SdResult::Back;
        } else {
            mc_dbg_350_sd_enter_cnt = mc_dbg_350_sd_enter_cnt + 1U;
            mc_dbg_351_sd_enter_idx = g_sd_sel;
        }
    }

    const uint8_t cycle = g_sd_live_cycle;
    g_sd_live_cycle = static_cast<uint8_t>((g_sd_live_cycle + 1U) % 2U);

    if (cycle == 0U) {
        const uint8_t st_code = read_sd_status_code();
        if (st_code != g_cached_status_code) {
            draw_status_card();
            draw_file_list_area();
        }
    }

    return SdResult::Stay;
}

}  // namespace mc
