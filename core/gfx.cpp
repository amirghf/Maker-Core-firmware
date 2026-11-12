// ابزار گرافیکی ساده روی hal_display. بدون حافظه پویا.
#include "gfx.h"
#include "gfx_font.h"
#include "hal_display.h"

namespace mc {
namespace {

constexpr uint8_t  kMaxScale  = 4U;
constexpr uint16_t kCellW     = 6U;   // ۵ ستون فونت + ۱ ستون فاصله
constexpr uint16_t kCellH     = 8U;   // ۷ سطر فونت + ۱ سطر فاصله

// بافر ثابت برای یک کاراکتر در بیشترین بزرگ‌نمایی (۲۴ در ۳۲ پیکسل)
uint16_t g_glyph[(kCellW * kMaxScale) * (kCellH * kMaxScale)];

uint8_t clamp_scale(uint8_t s) {
    if (s < 1U) { return 1U; }
    if (s > kMaxScale) { return kMaxScale; }
    return s;
}

}  // namespace

void mc_gfx_pixel(uint16_t x, uint16_t y, uint16_t color) {
    mc_display_fill_rect(x, y, 1U, 1U, color);
}

void mc_gfx_hline(uint16_t x, uint16_t y, uint16_t len, uint16_t color) {
    mc_display_fill_rect(x, y, len, 1U, color);
}

void mc_gfx_vline(uint16_t x, uint16_t y, uint16_t len, uint16_t color) {
    mc_display_fill_rect(x, y, 1U, len, color);
}

void mc_gfx_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    mc_display_fill_rect(x, y, w, h, color);
}

void mc_gfx_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if ((w == 0U) || (h == 0U)) { return; }
    mc_gfx_hline(x, y, w, color);                                                    // بالا
    mc_gfx_hline(x, static_cast<uint16_t>(y + h - 1U), w, color);                    // پایین
    mc_gfx_vline(x, y, h, color);                                                    // چپ
    mc_gfx_vline(static_cast<uint16_t>(x + w - 1U), y, h, color);                    // راست
}

void mc_gfx_char(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg, uint8_t scale) {
    const uint8_t s = clamp_scale(scale);
    uint8_t code = static_cast<uint8_t>(c);
    if ((code < kFontFirstChar) || (code > kFontLastChar)) { code = static_cast<uint8_t>('?'); }
    const uint8_t* glyph = &kFont5x7[static_cast<uint16_t>(code - kFontFirstChar) * 5U];

    const uint16_t w = static_cast<uint16_t>(kCellW * s);
    const uint16_t h = static_cast<uint16_t>(kCellH * s);
    if ((static_cast<uint32_t>(x) + w) > mc_display_width())  { return; }
    if ((static_cast<uint32_t>(y) + h) > mc_display_height()) { return; }

    for (uint16_t py = 0U; py < h; py = static_cast<uint16_t>(py + 1U)) {
        const uint16_t row = static_cast<uint16_t>(py / s);        // سطر فونت (۰ تا ۷)
        for (uint16_t px = 0U; px < w; px = static_cast<uint16_t>(px + 1U)) {
            const uint16_t col = static_cast<uint16_t>(px / s);    // ستون فونت (۰ تا ۵)
            bool on = false;
            if ((col < 5U) && (row < 7U)) {
                on = ((glyph[col] >> row) & 1U) != 0U;
            }
            g_glyph[(static_cast<uint32_t>(py) * w) + px] = on ? fg : bg;
        }
    }
    mc_display_write_pixels(x, y, w, h, g_glyph);
}

void mc_gfx_text(uint16_t x, uint16_t y, const char* text, uint16_t fg, uint16_t bg, uint8_t scale) {
    if (text == nullptr) { return; }
    const uint8_t s = clamp_scale(scale);
    const uint16_t cell_w = static_cast<uint16_t>(kCellW * s);
    const uint16_t cell_h = static_cast<uint16_t>(kCellH * s);
    uint16_t cx = x;
    uint16_t cy = y;
    for (const char* p = text; *p != '\0'; ++p) {
        if (*p == '\n') {
            cx = x;
            cy = static_cast<uint16_t>(cy + cell_h);
            continue;
        }
        if ((static_cast<uint32_t>(cx) + cell_w) > mc_display_width()) { continue; }   // بیرون از لبه راست
        mc_gfx_char(cx, cy, *p, fg, bg, s);
        cx = static_cast<uint16_t>(cx + cell_w);
    }
}

uint16_t mc_gfx_text_width(const char* text, uint8_t scale) {
    if (text == nullptr) { return 0U; }
    const uint8_t s = clamp_scale(scale);
    uint16_t n = 0U;
    for (const char* p = text; (*p != '\0') && (*p != '\n'); ++p) { n = static_cast<uint16_t>(n + 1U); }
    return static_cast<uint16_t>(n * kCellW * s);
}

}  // namespace mc
