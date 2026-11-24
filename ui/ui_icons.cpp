// آیکون‌ها و توابع ترسیمی گرافیکی رابط کاربری (بند ۸.۷). هیچ هدر ST اینجا include نمی‌شود.
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

// مستطیل پر با گوشه گرد شعاع r. فقط با عدد صحیح.
void mc_ui_fill_rrect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color) {
    if ((w == 0U) || (h == 0U)) {
        return;
    }
    if (r * 2U > w) {
        r = w / 2U;
    }
    if (r * 2U > h) {
        r = h / 2U;
    }
    const int32_t rr = static_cast<int32_t>(r) * static_cast<int32_t>(r);

    for (uint16_t dy = 0U; dy < h; ++dy) {
        uint16_t inset = 0U;
        if (r > 0U) {
            int32_t dy2 = -1;
            if (dy < r) {
                dy2 = static_cast<int32_t>(r - 1U - dy);
            } else if (dy >= h - r) {
                dy2 = static_cast<int32_t>(dy - (h - r));
            }
            if (dy2 >= 0) {
                int32_t dx = static_cast<int32_t>(r);
                while ((dx > 0) && ((dx * dx + dy2 * dy2) > rr)) {
                    --dx;
                }
                inset = static_cast<uint16_t>(static_cast<int32_t>(r) - dx);
            }
        }
        if (w > 2U * inset) {
            mc_gfx_hline(x + inset, y + dy, w - 2U * inset, color);
        }
    }
}

// کارت گوشه‌گرد
void mc_ui_card(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t fill, uint16_t border) {
    mc_ui_fill_rrect(x, y, w, h, r, border);
    if ((w > 2U) && (h > 2U)) {
        const uint16_t inner_r = (r > 0U) ? static_cast<uint16_t>(r - 1U) : 0U;
        mc_ui_fill_rrect(x + 1U, y + 1U, w - 2U, h - 2U, inner_r, fill);
    }
}

// مثلث پر با محاسبات عدد صحیح int32_t (edge function)
void mc_ui_fill_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    int16_t min_x = x0;
    if (x1 < min_x) { min_x = x1; }
    if (x2 < min_x) { min_x = x2; }

    int16_t max_x = x0;
    if (x1 > max_x) { max_x = x1; }
    if (x2 > max_x) { max_x = x2; }

    int16_t min_y = y0;
    if (y1 < min_y) { min_y = y1; }
    if (y2 < min_y) { min_y = y2; }

    int16_t max_y = y0;
    if (y1 > max_y) { max_y = y1; }
    if (y2 > max_y) { max_y = y2; }

    if (min_x < 0) { min_x = 0; }
    if (min_y < 0) { min_y = 0; }

    const int32_t x0_32 = x0; const int32_t y0_32 = y0;
    const int32_t x1_32 = x1; const int32_t y1_32 = y1;
    const int32_t x2_32 = x2; const int32_t y2_32 = y2;

    for (int16_t py = min_y; py <= max_y; ++py) {
        const int32_t py_32 = py;
        int16_t run_start = -1;

        for (int16_t px = min_x; px <= max_x; ++px) {
            const int32_t px_32 = px;
            const int32_t e0 = (x1_32 - x0_32) * (py_32 - y0_32) - (y1_32 - y0_32) * (px_32 - x0_32);
            const int32_t e1 = (x2_32 - x1_32) * (py_32 - y1_32) - (y2_32 - y1_32) * (px_32 - x1_32);
            const int32_t e2 = (x0_32 - x2_32) * (py_32 - y2_32) - (y0_32 - y2_32) * (px_32 - x2_32);

            const bool inside = ((e0 >= 0) && (e1 >= 0) && (e2 >= 0)) ||
                                ((e0 <= 0) && (e1 <= 0) && (e2 <= 0));

            if (inside) {
                if (run_start < 0) {
                    run_start = px;
                }
            } else {
                if (run_start >= 0) {
                    mc_gfx_hline(static_cast<uint16_t>(run_start), static_cast<uint16_t>(py),
                                 static_cast<uint16_t>(px - run_start), color);
                    run_start = -1;
                }
            }
        }
        if (run_start >= 0) {
            mc_gfx_hline(static_cast<uint16_t>(run_start), static_cast<uint16_t>(py),
                         static_cast<uint16_t>(max_x + 1 - run_start), color);
        }
    }
}

// نوار پیشرفت
void mc_ui_progress(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t fill_px, uint16_t fill_color, uint16_t track_color) {
    if (fill_px > w) {
        fill_px = w;
    }
    if (fill_px > 0U) {
        mc_gfx_fill_rect(x, y, fill_px, h, fill_color);
    }
    if (fill_px < w) {
        mc_gfx_fill_rect(x + fill_px, y, w - fill_px, h, track_color);
    }
}

// آیکون جهت موتور (جعبه ۱۲ در ۱۲)
void mc_icon_dir(uint16_t x, uint16_t y, bool positive, uint16_t color) {
    if (positive) {
        // مثلث رو به بالا
        mc_ui_fill_triangle(static_cast<int16_t>(x + 6U), static_cast<int16_t>(y + 1U),
                            static_cast<int16_t>(x + 1U), static_cast<int16_t>(y + 10U),
                            static_cast<int16_t>(x + 11U), static_cast<int16_t>(y + 10U), color);
    } else {
        // مثلث رو به پایین
        mc_ui_fill_triangle(static_cast<int16_t>(x + 1U), static_cast<int16_t>(y + 2U),
                            static_cast<int16_t>(x + 11U), static_cast<int16_t>(y + 2U),
                            static_cast<int16_t>(x + 6U), static_cast<int16_t>(y + 11U), color);
    }
}

// آیکون جفت‌مثلث جهت موتور بدون دم (جعبه ۱۴ در ۲۲، بند ۸.۷)
void mc_icon_dir_pair(uint16_t x, uint16_t y, uint8_t active, uint16_t active_color, uint16_t dim_color) {
    const uint16_t top_color    = (active == 1U) ? active_color : dim_color;
    const uint16_t bottom_color = (active == 2U) ? active_color : dim_color;

    // مثلث بالا (جهت مثبت): نقاط (x, y+9)، (x+14, y+9)، (x+7, y)
    mc_ui_fill_triangle(static_cast<int16_t>(x), static_cast<int16_t>(y + 9U),
                        static_cast<int16_t>(x + 14U), static_cast<int16_t>(y + 9U),
                        static_cast<int16_t>(x + 7U), static_cast<int16_t>(y), top_color);

    // مثلث پایین (جهت منفی): نقاط (x, y+12)، (x+14, y+12)، (x+7, y+21)
    mc_ui_fill_triangle(static_cast<int16_t>(x), static_cast<int16_t>(y + 12U),
                        static_cast<int16_t>(x + 14U), static_cast<int16_t>(y + 12U),
                        static_cast<int16_t>(x + 7U), static_cast<int16_t>(y + 21U), bottom_color);
}

void mc_icon_dir_pair_v(uint16_t x, uint16_t y, uint8_t mode, uint16_t top_color, uint16_t bottom_color, uint16_t dim_color) {
    const uint16_t top_col = ((mode & 1U) != 0U) ? top_color : dim_color;
    const uint16_t bot_col = ((mode & 2U) != 0U) ? bottom_color : dim_color;

    // مثلث بالا (جهت مثبت): نقاط (x, y+9)، (x+14, y+9)، (x+7, y)
    mc_ui_fill_triangle(static_cast<int16_t>(x), static_cast<int16_t>(y + 9U),
                        static_cast<int16_t>(x + 14U), static_cast<int16_t>(y + 9U),
                        static_cast<int16_t>(x + 7U), static_cast<int16_t>(y), top_col);

    // مثلث پایین (جهت منفی): نقاط (x, y+12)، (x+14, y+12)، (x+7, y+21)
    mc_ui_fill_triangle(static_cast<int16_t>(x), static_cast<int16_t>(y + 12U),
                        static_cast<int16_t>(x + 14U), static_cast<int16_t>(y + 12U),
                        static_cast<int16_t>(x + 7U), static_cast<int16_t>(y + 21U), bot_col);
}

void mc_icon_dir_pair_h(uint16_t x, uint16_t y, uint8_t mode, uint16_t right_color, uint16_t left_color, uint16_t dim_color) {
    const uint16_t right_col = ((mode & 1U) != 0U) ? right_color : dim_color;
    const uint16_t left_col  = ((mode & 2U) != 0U) ? left_color : dim_color;

    // مثلث چپ (جهت منفی): نقاط (x+9, y)، (x+9, y+14)، (x, y+7)
    mc_ui_fill_triangle(static_cast<int16_t>(x + 9U), static_cast<int16_t>(y),
                        static_cast<int16_t>(x + 9U), static_cast<int16_t>(y + 14U),
                        static_cast<int16_t>(x), static_cast<int16_t>(y + 7U), left_col);

    // مثلث راست (جهت مثبت): نقاط (x+12, y)، (x+12, y+14)، (x+21, y+7)
    mc_ui_fill_triangle(static_cast<int16_t>(x + 12U), static_cast<int16_t>(y),
                        static_cast<int16_t>(x + 12U), static_cast<int16_t>(y + 14U),
                        static_cast<int16_t>(x + 21U), static_cast<int16_t>(y + 7U), right_col);
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

// نازل اکسترودر (۳۲ در ۳۲ دورنگ)
void mc_icon_nozzle(uint16_t x, uint16_t y, uint16_t accent, uint16_t body) {
    // بدنه هیتر
    mc_ui_fill_rrect(x + 9U, y + 2U, 14U, 12U, 2U, body);
    // دو پره گرما
    mc_gfx_fill_rect(x + 11U, y + 5U, 10U, 2U, accent);
    mc_gfx_fill_rect(x + 11U, y + 9U, 10U, 2U, accent);
    // گردن
    mc_gfx_fill_rect(x + 13U, y + 14U, 6U, 4U, body);
    // نوک مخروطی
    mc_ui_fill_triangle(static_cast<int16_t>(x + 11U), static_cast<int16_t>(y + 18U),
                        static_cast<int16_t>(x + 21U), static_cast<int16_t>(y + 18U),
                        static_cast<int16_t>(x + 16U), static_cast<int16_t>(y + 28U), accent);
    // قطره
    mc_ui_fill_circle(x + 16U, y + 30U, 1U, accent);
}

// فن (۳۲ در ۳۲ دورنگ، طرح جدید)
void mc_icon_fan(uint16_t x, uint16_t y, uint16_t accent, uint16_t body, uint16_t bg) {
    // ۱. قاب
    mc_ui_fill_rrect(x, y, 32U, 32U, 6U, body);
    // ۲. چهار پیچ گوشه‌ها
    mc_gfx_fill_rect(x + 3U, y + 3U, 2U, 2U, bg);
    mc_gfx_fill_rect(x + 27U, y + 3U, 2U, 2U, bg);
    mc_gfx_fill_rect(x + 3U, y + 27U, 2U, 2U, bg);
    mc_gfx_fill_rect(x + 27U, y + 27U, 2U, 2U, bg);
    // ۳. حلقه رنگی
    mc_ui_fill_circle(x + 16U, y + 16U, 14U, accent);
    mc_ui_fill_circle(x + 16U, y + 16U, 13U, body);
    // ۴. دیسک داخلی
    mc_ui_fill_circle(x + 16U, y + 16U, 12U, bg);
    // ۵. چهار پره
    mc_ui_fill_triangle(static_cast<int16_t>(x + 16U), static_cast<int16_t>(y + 16U),
                        static_cast<int16_t>(x + 15U), static_cast<int16_t>(y + 5U),
                        static_cast<int16_t>(x + 25U), static_cast<int16_t>(y + 9U), accent);
    mc_ui_fill_triangle(static_cast<int16_t>(x + 16U), static_cast<int16_t>(y + 16U),
                        static_cast<int16_t>(x + 27U), static_cast<int16_t>(y + 15U),
                        static_cast<int16_t>(x + 23U), static_cast<int16_t>(y + 25U), accent);
    mc_ui_fill_triangle(static_cast<int16_t>(x + 16U), static_cast<int16_t>(y + 16U),
                        static_cast<int16_t>(x + 17U), static_cast<int16_t>(y + 27U),
                        static_cast<int16_t>(x + 7U), static_cast<int16_t>(y + 23U), accent);
    mc_ui_fill_triangle(static_cast<int16_t>(x + 16U), static_cast<int16_t>(y + 16U),
                        static_cast<int16_t>(x + 5U), static_cast<int16_t>(y + 17U),
                        static_cast<int16_t>(x + 9U), static_cast<int16_t>(y + 7U), accent);
    // ۶. هاب
    mc_ui_fill_circle(x + 16U, y + 16U, 4U, 0xFFFFU);
    mc_ui_fill_circle(x + 16U, y + 16U, 1U, body);
}

// بستر داغ (۳۲ در ۳۲ دورنگ)
void mc_icon_bed(uint16_t x, uint16_t y, uint16_t accent, uint16_t body) {
    // صفحه
    mc_ui_fill_rrect(x + 2U, y + 18U, 28U, 6U, 2U, body);
    // دو پایه
    mc_gfx_fill_rect(x + 6U, y + 24U, 4U, 6U, body);
    mc_gfx_fill_rect(x + 22U, y + 24U, 4U, 6U, body);
    // سه موج گرما
    for (uint16_t i = 0U; i < 3U; ++i) {
        const uint16_t bx = static_cast<uint16_t>(x + 8U + i * 7U);
        mc_gfx_fill_rect(bx + 1U, y + 3U, 2U, 4U, accent);
        mc_gfx_fill_rect(bx, y + 7U, 2U, 4U, accent);
        mc_gfx_fill_rect(bx + 1U, y + 11U, 2U, 4U, accent);
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
    // سر پیکان
    for (uint16_t row = 1U; row <= 11U; ++row) {
        const int32_t dy = (row >= 6U) ? static_cast<int32_t>(row - 6U) : static_cast<int32_t>(6U - row);
        int32_t w = (8 * (6 - dy)) / 6;
        if (w < 1) {
            w = 1;
        }
        mc_gfx_hline(x + 16U, y + row, static_cast<uint16_t>(w), color);
    }
}

void mc_icon_menu_item(uint16_t x, uint16_t y, uint8_t id, uint16_t color, uint16_t bg) {
    switch (id) {
        case 0: // MOVE: چهار مثلث به چهار جهت دور یک نقطه در جعبه ۳۲ در ۳۲
            mc_ui_fill_circle(x + 16U, y + 16U, 2U, color);
            mc_ui_fill_triangle(static_cast<int16_t>(x + 16U), static_cast<int16_t>(y + 3U),
                                static_cast<int16_t>(x + 10U), static_cast<int16_t>(y + 11U),
                                static_cast<int16_t>(x + 22U), static_cast<int16_t>(y + 11U), color);
            mc_ui_fill_triangle(static_cast<int16_t>(x + 16U), static_cast<int16_t>(y + 29U),
                                static_cast<int16_t>(x + 10U), static_cast<int16_t>(y + 21U),
                                static_cast<int16_t>(x + 22U), static_cast<int16_t>(y + 21U), color);
            mc_ui_fill_triangle(static_cast<int16_t>(x + 3U), static_cast<int16_t>(y + 16U),
                                static_cast<int16_t>(x + 11U), static_cast<int16_t>(y + 10U),
                                static_cast<int16_t>(x + 11U), static_cast<int16_t>(y + 22U), color);
            mc_ui_fill_triangle(static_cast<int16_t>(x + 29U), static_cast<int16_t>(y + 16U),
                                static_cast<int16_t>(x + 21U), static_cast<int16_t>(y + 10U),
                                static_cast<int16_t>(x + 21U), static_cast<int16_t>(y + 22U), color);
            break;

        case 1: // EXTRUDER
            mc_icon_nozzle(x, y, color, mc_rgb565(120, 135, 165));
            break;

        case 2: // HEAT
            mc_icon_bed(x, y, color, mc_rgb565(120, 135, 165));
            break;

        case 3: // SENSORS: دایره پر در پایین و میله عمودی شبیه دماسنج
            mc_ui_fill_circle(x + 16U, y + 23U, 5U, color);
            mc_gfx_fill_rect(x + 14U, y + 5U, 5U, 15U, color);
            mc_gfx_hline(x + 21U, y + 8U, 4U, color);
            mc_gfx_hline(x + 21U, y + 13U, 4U, color);
            mc_gfx_hline(x + 21U, y + 18U, 4U, color);
            break;

        case 4: // SD CARD: آیکون ۲۴ در ۲۴ با جابه‌جایی +۴ در جعبه ۳۲
            mc_icon_sd(x + 4U, y + 4U, color, bg);
            break;

        case 5: // USB OTG: آیکون ۲۴ در ۲۴ با جابه‌جایی +۴
            mc_icon_usb(x + 4U, y + 4U, color, bg);
            break;

        case 6: // ABOUT: دایره توپر و حرف i داخلش
            mc_ui_fill_circle(x + 16U, y + 16U, 13U, color);
            mc_gfx_fill_rect(x + 14U, y + 7U, 4U, 4U, bg);
            mc_gfx_fill_rect(x + 14U, y + 14U, 4U, 11U, bg);
            break;

        case 7: // BACK: مثلث رو به چپ
            mc_ui_fill_triangle(static_cast<int16_t>(x + 8U), static_cast<int16_t>(y + 16U),
                                static_cast<int16_t>(x + 24U), static_cast<int16_t>(y + 6U),
                                static_cast<int16_t>(x + 24U), static_cast<int16_t>(y + 26U), color);
            break;

        default:
            break;
    }
}

}  // namespace mc
