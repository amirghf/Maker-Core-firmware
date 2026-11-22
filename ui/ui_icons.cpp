// آیکون‌های رابط کاربری (بند ۸.۷). هیچ هدر ST اینجا include نمی‌شود.
#include "ui_icons.h"
#include "../core/gfx.h"

namespace mc {

void mc_ui_fill_circle(uint16_t cx, uint16_t cy, uint16_t r, uint16_t color) {
    const int32_t rr = static_cast<int32_t>(r) * static_cast<int32_t>(r);
    for (int32_t dy = -static_cast<int32_t>(r); dy <= static_cast<int32_t>(r); ++dy) {
        int32_t dx = static_cast<int32_t>(r);
        while ((dx > 0) && ((dx * dx + dy * dy) > rr)) {
            --dx;
        }
        const int32_t px = static_cast<int32_t>(cx) - dx;
        const int32_t py = static_cast<int32_t>(cy) + dy;
        if ((px < 0) || (py < 0)) {
            continue;
        }
        mc_gfx_hline(static_cast<uint16_t>(px), static_cast<uint16_t>(py),
                     static_cast<uint16_t>(2 * dx + 1), color);
    }
}

// کارت SD: بدنه با گوشه بریده بالا-راست و سه شیار تماس
void mc_icon_sd(uint16_t x, uint16_t y, uint16_t color, uint16_t bg) {
    mc_gfx_fill_rect(x + 4U, y + 2U, 16U, 20U, color);
    for (uint16_t r = 0U; r < 4U; ++r) {
        mc_gfx_hline(x + 16U + r, y + 2U + r, 4U - r, bg);
    }
    mc_gfx_fill_rect(x + 6U, y + 3U, 2U, 6U, bg);
    mc_gfx_fill_rect(x + 10U, y + 3U, 2U, 6U, bg);
    mc_gfx_fill_rect(x + 14U, y + 3U, 1U, 6U, bg);
}

// دوشاخه USB: سر دوشاخه با دو سوراخ، بدنه و کابل
void mc_icon_usb(uint16_t x, uint16_t y, uint16_t color, uint16_t bg) {
    mc_gfx_fill_rect(x + 7U, y + 2U, 10U, 8U, color);
    mc_gfx_fill_rect(x + 9U, y + 4U, 2U, 2U, bg);
    mc_gfx_fill_rect(x + 13U, y + 4U, 2U, 2U, bg);
    mc_gfx_fill_rect(x + 5U, y + 10U, 14U, 10U, color);
    mc_gfx_fill_rect(x + 10U, y + 20U, 4U, 3U, color);
}

// منو: سه خط افقی
void mc_icon_menu(uint16_t x, uint16_t y, uint16_t color) {
    mc_gfx_fill_rect(x + 3U, y + 5U, 18U, 3U, color);
    mc_gfx_fill_rect(x + 3U, y + 10U, 18U, 3U, color);
    mc_gfx_fill_rect(x + 3U, y + 15U, 18U, 3U, color);
}

// نازل: بلوک هیتر، گردن و نوک مخروطی
void mc_icon_nozzle(uint16_t x, uint16_t y, uint16_t color) {
    mc_gfx_fill_rect(x + 6U, y + 3U, 12U, 9U, color);
    mc_gfx_fill_rect(x + 9U, y + 12U, 6U, 3U, color);
    for (uint16_t r = 0U; r < 6U; ++r) {
        const uint16_t k = static_cast<uint16_t>(r / 2U);
        mc_gfx_hline(x + 9U + k, y + 15U + r, 6U - 2U * k, color);
    }
}

// فن: قاب مربعی، چهار پره و محور
void mc_icon_fan(uint16_t x, uint16_t y, uint16_t color, uint16_t bg) {
    mc_gfx_rect(x + 1U, y + 1U, 22U, 22U, color);
    mc_gfx_fill_rect(x + 10U, y + 3U, 5U, 7U, color);
    mc_gfx_fill_rect(x + 9U, y + 14U, 5U, 7U, color);
    mc_gfx_fill_rect(x + 3U, y + 9U, 7U, 5U, color);
    mc_gfx_fill_rect(x + 14U, y + 10U, 7U, 5U, color);
    mc_ui_fill_circle(x + 12U, y + 12U, 3U, color);
    mc_ui_fill_circle(x + 12U, y + 12U, 1U, bg);
}

// بستر داغ: صفحه تخت، پایه و سه موج گرما
void mc_icon_bed(uint16_t x, uint16_t y, uint16_t color) {
    mc_gfx_fill_rect(x + 2U, y + 14U, 20U, 4U, color);
    mc_gfx_fill_rect(x + 4U, y + 18U, 3U, 4U, color);
    mc_gfx_fill_rect(x + 17U, y + 18U, 3U, 4U, color);
    for (uint16_t i = 0U; i < 3U; ++i) {
        const uint16_t bx = static_cast<uint16_t>(x + 6U + i * 5U);
        mc_gfx_vline(bx + 1U, y + 2U, 3U, color);
        mc_gfx_vline(bx, y + 5U, 3U, color);
        mc_gfx_vline(bx + 1U, y + 8U, 3U, color);
    }
}

// play: مثلث رو به راست (جعبه ۳۲ در ۳۲)
void mc_icon_play(uint16_t x, uint16_t y, uint16_t color) {
    for (uint16_t row = 2U; row <= 30U; ++row) {
        const int32_t dy = (row >= 16U) ? static_cast<int32_t>(row - 16U) : static_cast<int32_t>(16U - row);
        int32_t w = (20 * (14 - dy)) / 14;
        if (w < 1) {
            w = 1;
        }
        mc_gfx_hline(x + 8U, y + row, static_cast<uint16_t>(w), color);
    }
}

// pause: دو میله عمودی
void mc_icon_pause(uint16_t x, uint16_t y, uint16_t color) {
    mc_gfx_fill_rect(x + 7U, y + 4U, 7U, 24U, color);
    mc_gfx_fill_rect(x + 18U, y + 4U, 7U, 24U, color);
}

// stop: مربع پر
void mc_icon_stop(uint16_t x, uint16_t y, uint16_t color) {
    mc_gfx_fill_rect(x + 6U, y + 6U, 20U, 20U, color);
}

// reset: حلقه با دهانه بالا-راست و سر پیکان رو به راست
void mc_icon_reset(uint16_t x, uint16_t y, uint16_t color) {
    for (int32_t py = 0; py < 32; ++py) {
        const int32_t dy = py - 16;
        int32_t run_start = -1;
        for (int32_t px = 0; px <= 32; ++px) {
            bool on = false;
            if (px < 32) {
                const int32_t dx = px - 16;
                const int32_t d2 = dx * dx + dy * dy;
                const bool in_ring = (d2 >= 64) && (d2 <= 144);
                const bool in_gap = (dx >= 2) && (dy <= -2);
                on = in_ring && !in_gap;
            }
            if (on && (run_start < 0)) {
                run_start = px;
            } else if (!on && (run_start >= 0)) {
                mc_gfx_hline(static_cast<uint16_t>(x + static_cast<uint16_t>(run_start)),
                             static_cast<uint16_t>(y + static_cast<uint16_t>(py)),
                             static_cast<uint16_t>(px - run_start), color);
                run_start = -1;
            }
        }
    }
    // سر پیکان: قاعده عمودی در x=16 از y=1 تا y=11، نوک در x=24
    for (uint16_t row = 1U; row <= 11U; ++row) {
        const int32_t dy = (row >= 6U) ? static_cast<int32_t>(row - 6U) : static_cast<int32_t>(6U - row);
        int32_t w = (8 * (6 - dy)) / 6;
        if (w < 1) {
            w = 1;
        }
        mc_gfx_hline(x + 16U, y + row, static_cast<uint16_t>(w), color);
    }
}

}  // namespace mc
