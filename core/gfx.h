#pragma once
// ابزار گرافیکی ساده. فقط از hal_display استفاده می‌کند. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

namespace mc {

// ساخت رنگ RGB565 از سه عدد ۰ تا ۲۵۵.
constexpr uint16_t mc_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r & 0xF8U) << 8) | ((g & 0xFCU) << 3) | (b >> 3));
}

// یک پیکسل. بیرون از صفحه نادیده گرفته می‌شود.
void mc_gfx_pixel(uint16_t x, uint16_t y, uint16_t color);
// خط افقی و عمودی. بخش بیرون از صفحه بریده می‌شود.
void mc_gfx_hline(uint16_t x, uint16_t y, uint16_t len, uint16_t color);
void mc_gfx_vline(uint16_t x, uint16_t y, uint16_t len, uint16_t color);
// مستطیل توخالی با ضخامت ۱ پیکسل.
void mc_gfx_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
// مستطیل پر.
void mc_gfx_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

// یک کاراکتر ASCII با بزرگ‌نمایی scale (۱ تا ۴). خانه کاراکتر ۶ در ۸ پیکسل ضرب در scale است
// (ستون و سطر آخر فاصله‌اند). رنگ پس‌زمینه bg پر می‌شود. کاراکتر نامعتبر به صورت « ? » نوشته می‌شود.
void mc_gfx_char(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg, uint8_t scale);
// یک رشته. کاراکتر '\n' به سطر بعدی می‌رود. اگر رشته به لبه راست برسد ادامه‌اش بریده می‌شود.
void mc_gfx_text(uint16_t x, uint16_t y, const char* text, uint16_t fg, uint16_t bg, uint8_t scale);
// عرض رشته (تا اولین '\n' یا پایان) به پیکسل.
uint16_t mc_gfx_text_width(const char* text, uint8_t scale);

}  // namespace mc
