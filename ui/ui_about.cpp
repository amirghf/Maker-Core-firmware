// صفحه ABOUT رابط کاربری (بند ۸.۱۸).
// هیچ هدر ST اینجا include نمی‌شود. فقط از hal و gfx و encoder استفاده می‌کند.
#include "ui_about.h"
#include "ui_home.h"
#include "ui_colors.h"
#include "../core/gfx.h"
#include "../core/text_format.h"
#include "../core/encoder.h"
#include "../hal/hal_display.h"
#include "../config/mc_version.h"

extern "C" {
volatile uint32_t mc_dbg_360_about_sel_shown = 0; // خواندنی: اندیس مورد فوکوس‌شده (همیشه ۰ = BACK)
volatile uint32_t mc_dbg_361_about_force     = 0; // نوشتنی: ۰ = بی‌اثر؛ ۱ = باز کردن صفحه؛ ۲ = برگشت به منو
}

namespace mc {
namespace {

constexpr uint16_t kSelBg      = mc_rgb565(24, 38, 64);
constexpr uint16_t kAccAbout   = kAccFan;                // همان رنگ آیتم ABOUT در منو (0, 200, 230)
constexpr uint16_t kLogoBg     = mc_rgb565(22, 30, 48);
constexpr uint16_t kLogoOrange = mc_rgb565(255, 145, 25);
constexpr uint16_t kLogoWhite  = kColorWhite;

static uint8_t g_about_step = 0U; // گام نقاشی ورود (۰ تا ۵، ۲۵۵ = کار عادی)

// محاسبه جذر صحیح (بدون float)
static int16_t isqrt(int32_t n) {
    if (n <= 0) { return 0; }
    int32_t x = 0;
    while ((x + 1) * (x + 1) <= n) {
        x++;
    }
    return static_cast<int16_t>(x);
}

// رسم دیسک پر با قطر 2*r+1
static void draw_disc(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
    for (int16_t dy = -r; dy <= r; ++dy) {
        const int16_t dx = isqrt(static_cast<int32_t>(r * r - dy * dy));
        mc_gfx_hline(static_cast<uint16_t>(cx - dx),
                     static_cast<uint16_t>(cy + dy),
                     static_cast<uint16_t>(2 * dx + 1),
                     color);
    }
}

// رسم خط ضخیم با الگوریتم صحیح Bresenham (بدون float) که در هر نقطه یک مربع (2*half+1)x(2*half+1) می‌گذارد
static void draw_thick_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t half, uint16_t color) {
    int16_t dx = (x0 < x1) ? (x1 - x0) : (x0 - x1);
    int16_t dy = (y0 < y1) ? (y1 - y0) : (y0 - y1);
    const int16_t sx = (x0 < x1) ? 1 : -1;
    const int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;

    const uint16_t sz = static_cast<uint16_t>(2 * half + 1);

    while (true) {
        mc_gfx_fill_rect(static_cast<uint16_t>(x0 - half),
                         static_cast<uint16_t>(y0 - half),
                         sz, sz, color);
        if (x0 == x1 && y0 == y1) { break; }
        const int16_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// رسم نوار بالا
static void draw_top_bar() {
    mc_gfx_fill_rect(0U, 0U, 480U, 36U, kTopBarBg);
    mc_gfx_fill_rect(0U, 36U, 480U, 2U, kAccentLine);
    const uint16_t tw = mc_gfx_text_width("ABOUT", 2U);
    const uint16_t tx = static_cast<uint16_t>((480U - tw) / 2U);
    mc_gfx_text(tx, 10U, "ABOUT", kColorWhite, kTopBarBg, 2U);
}

// رسم کارت اطلاعات و ۵ ردیف آن
static void draw_info_card() {
    constexpr uint16_t bx = 152U;
    constexpr uint16_t by = 46U;
    constexpr uint16_t bw = 320U;
    constexpr uint16_t bh = 198U;

    mc_gfx_fill_rect(bx, by, bw, bh, kCardBg);
    mc_gfx_rect(bx, by, bw, bh, kCardBorder);

    // نوار رنگی سمت چپ
    mc_gfx_fill_rect(bx, by, 4U, bh, kAccAbout);

    static const char* const kLabels[5] = {
        "FRAMEWORK",
        "VERSION",
        "DEVELOPER",
        "YEAR",
        "BOARD"
    };

    static const char* const kValues[5] = {
        MC_FIRMWARE_NAME,
        MC_VERSION_STRING,
        "AMIR GHARZI",
        "1405",
        "MW-X1"
    };

    for (uint8_t i = 0U; i < 5U; ++i) {
        const uint16_t row_top = static_cast<uint16_t>(50U + 38U * i);

        // برچسب scale 1
        mc_gfx_text(166U, static_cast<uint16_t>(row_top + 3U), kLabels[i], kLabelGray, kCardBg, 1U);
        // مقدار scale 2
        mc_gfx_text(166U, static_cast<uint16_t>(row_top + 15U), kValues[i], kColorWhite, kCardBg, 2U);

        // خط جداکننده برای ردیف‌های ۰ تا ۳
        if (i < 4U) {
            mc_gfx_hline(160U, static_cast<uint16_t>(row_top + 36U), 304U, kCardBorder);
        }
    }
}

// رسم بخش اول آیکون: زمینه، شش‌ضلعی و گره‌های رأس
static void draw_icon_part1() {
    constexpr int16_t ox = 20;
    constexpr int16_t oy = 89;

    // الف) زمینه: مربع گوشه‌گرد 112 در 112
    static const uint8_t kInsets[22] = {
        18U, 15U, 12U, 11U, 9U, 8U, 7U, 6U, 5U, 4U, 4U,
        3U,  3U,  2U,  2U,  1U, 1U, 1U, 1U, 1U, 1U, 1U
    };

    // ردیف‌های ۰ تا ۲۱
    for (uint8_t y = 0U; y < 22U; ++y) {
        const uint8_t inset = kInsets[y];
        const uint16_t hx = static_cast<uint16_t>(ox + inset);
        const uint16_t hy = static_cast<uint16_t>(oy + y);
        const uint16_t hw = static_cast<uint16_t>(112U - 2U * inset);
        mc_gfx_hline(hx, hy, hw, kLogoBg);
    }

    // ردیف‌های ۲۲ تا ۸۹ کل عرض ۰ تا ۱۱۱ یک‌جا
    mc_gfx_fill_rect(static_cast<uint16_t>(ox), static_cast<uint16_t>(oy + 22), 112U, 68U, kLogoBg);

    // ردیف‌های ۹۰ تا ۱۱۱ قرینه بالا
    for (uint8_t y = 90U; y < 112U; ++y) {
        const uint8_t top_y = static_cast<uint8_t>(111U - y);
        const uint8_t inset = kInsets[top_y];
        const uint16_t hx = static_cast<uint16_t>(ox + inset);
        const uint16_t hy = static_cast<uint16_t>(oy + y);
        const uint16_t hw = static_cast<uint16_t>(112U - 2U * inset);
        mc_gfx_hline(hx, hy, hw, kLogoBg);
    }

    // ب) شش‌ضلعی: رأس‌ها P0 تا P5
    static const int16_t kPx[6] = { 56, 92, 92, 56, 20, 20 };
    static const int16_t kPy[6] = { 14, 35, 77, 98, 77, 35 };

    for (uint8_t i = 0U; i < 6U; ++i) {
        const uint8_t next_i = static_cast<uint8_t>((i + 1U) % 6U);
        draw_thick_line(ox + kPx[i], oy + kPy[i],
                        ox + kPx[next_i], oy + kPy[next_i],
                        1, kLogoOrange);
    }

    // پ) گره‌های رأس: دیسک نارنجی به شعاع ۳ در هر شش رأس
    for (uint8_t i = 0U; i < 6U; ++i) {
        draw_disc(ox + kPx[i], oy + kPy[i], 3, kLogoOrange);
    }
}

// رسم بخش دوم آیکون: حرف M سفید و نقطه وسط
static void draw_icon_part2() {
    constexpr int16_t ox = 20;
    constexpr int16_t oy = 89;

    // ت) حرف M سفید:
    // ساق چپ عمودی از (33,43) تا (33,72)
    mc_gfx_fill_rect(static_cast<uint16_t>(ox + 33 - 3), static_cast<uint16_t>(oy + 43), 7U, 30U, kLogoWhite);
    // ساق راست عمودی از (79,43) تا (79,72)
    mc_gfx_fill_rect(static_cast<uint16_t>(ox + 79 - 3), static_cast<uint16_t>(oy + 43), 7U, 30U, kLogoWhite);

    // دو قطر: از (33,43) تا (56,61) و از (56,61) تا (79,43) با half=2 (ضخامت ۵)
    draw_thick_line(ox + 33, oy + 43, ox + 56, oy + 61, 2, kLogoWhite);
    draw_thick_line(ox + 56, oy + 61, ox + 79, oy + 43, 2, kLogoWhite);

    // دیسک سفید شعاع ۳ در سر و ته ساق‌ها و رأس وسط برای گرد شدن
    draw_disc(ox + 33, oy + 43, 3, kLogoWhite);
    draw_disc(ox + 33, oy + 72, 3, kLogoWhite);
    draw_disc(ox + 79, oy + 43, 3, kLogoWhite);
    draw_disc(ox + 79, oy + 72, 3, kLogoWhite);
    draw_disc(ox + 56, oy + 61, 3, kLogoWhite);

    // ث) نقطه وسط: دیسک نارنجی شعاع ۴ در (56,63)، بعد از M
    draw_disc(ox + 56, oy + 63, 4, kLogoOrange);
}

// رسم دکمه BACK
static void draw_back_button() {
    constexpr uint16_t bx = 324U;
    constexpr uint16_t by = 258U;
    constexpr uint16_t bw = 148U;
    constexpr uint16_t bh = 44U;

    mc_gfx_fill_rect(bx, by, bw, bh, kSelBg);
    mc_gfx_rect(bx, by, bw, bh, kAccentLine);
    mc_gfx_rect(static_cast<uint16_t>(bx + 1U), static_cast<uint16_t>(by + 1U),
                static_cast<uint16_t>(bw - 2U), static_cast<uint16_t>(bh - 2U), kAccentLine);

    const uint16_t tw = mc_gfx_text_width("BACK", 2U);
    const uint16_t tx = static_cast<uint16_t>(bx + (bw - tw) / 2U);
    const uint16_t ty = static_cast<uint16_t>(by + (bh - 16U) / 2U);
    mc_gfx_text(tx, ty, "BACK", kColorWhite, kSelBg, 2U);
}

}  // namespace

void mc_ui_about_open() {
    g_about_step = 0U;

    mc_dbg_360_about_sel_shown = 0U;
    mc_dbg_361_about_force     = 0U;

    (void)mc_encoder_take_clicks();
    (void)mc_encoder_take_press();
}

AboutResult mc_ui_about_poll() {
    (void)mc_dbg_360_about_sel_shown;
    (void)mc_dbg_361_about_force;

    if (mc_dbg_361_about_force != 0U) {
        const uint32_t force_val = mc_dbg_361_about_force;
        mc_dbg_361_about_force = 0U;

        if (force_val == 2U) {
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return AboutResult::Back;
        } else if (force_val == 1U) {
            g_about_step = 0U;
        }
    }

    if (g_about_step < 255U) {
        switch (g_about_step) {
            case 0:
                mc_display_fill_screen(kScreenBg);
                g_about_step = 1U;
                return AboutResult::Stay;

            case 1:
                draw_top_bar();
                g_about_step = 2U;
                return AboutResult::Stay;

            case 2:
                draw_info_card();
                g_about_step = 3U;
                return AboutResult::Stay;

            case 3:
                draw_icon_part1();
                g_about_step = 4U;
                return AboutResult::Stay;

            case 4:
                draw_icon_part2();
                g_about_step = 5U;
                return AboutResult::Stay;

            case 5:
                draw_back_button();
                g_about_step = 255U;
                return AboutResult::Stay;

            default:
                g_about_step = 255U;
                return AboutResult::Stay;
        }
    }

    (void)mc_encoder_take_clicks();

    if (mc_encoder_take_press()) {
        (void)mc_encoder_take_clicks();
        (void)mc_encoder_take_press();
        return AboutResult::Back;
    }

    return AboutResult::Stay;
}

}  // namespace mc
