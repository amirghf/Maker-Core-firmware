#include "splash.h"
#include "hal_display.h"
#include "hal_flash.h"
#include "hal_watchdog.h"
#include "gfx.h"

namespace mc {
namespace {

constexpr uint16_t kStripRows = 4U;                 // هر بار ۴ سطر خوانده و نوشته می‌شود
constexpr uint32_t kStripPixels = static_cast<uint32_t>(kSplashWidth) * kStripRows;
constexpr uint32_t kStripBytes  = kStripPixels * 2U;
// اگر رنگ‌ها روی نمایشگر اشتباه دیده شد، این را true کن (جابه‌جایی دو بایت هر پیکسل).
constexpr bool kSwapBytes = false;

static_assert((kSplashHeight % kStripRows) == 0U, "height must be a multiple of strip rows");

uint8_t  g_strip_bytes[kStripBytes];
uint16_t g_strip_pixels[kStripPixels];

// اگر لوگو نیامد، یک متن ساده نشان بده تا صفحه خالی نماند.
void show_fallback() {
    mc_display_fill_screen(kColorBlack);
    const uint16_t text_w = mc_gfx_text_width("MakerCore", 3U);
    const uint16_t w = mc_display_width();
    const uint16_t h = mc_display_height();
    const uint16_t x = (w > text_w) ? static_cast<uint16_t>((w - text_w) / 2U) : 0U;
    const uint16_t y = (h > 24U) ? static_cast<uint16_t>((h - 24U) / 2U) : 0U;
    mc_gfx_text(x, y, "MakerCore", kColorWhite, kColorBlack, 3U);
    mc_display_backlight(true);
}

}  // namespace

uint32_t mc_splash_show() {
    if (!mc_display_init()) { return 2U; }
    mc_display_fill_screen(kColorBlack);   // اول صفحه سیاه، بعد لوگو، بعد بک‌لایت

    if (!mc_flash_init()) {
        show_fallback();
        return 3U;
    }
    if ((mc_display_width() != kSplashWidth) || (mc_display_height() != kSplashHeight)) {
        show_fallback();
        return 4U;
    }

    for (uint16_t y = 0U; y < kSplashHeight; y = static_cast<uint16_t>(y + kStripRows)) {
        const uint32_t addr = kSplashFlashAddr + (static_cast<uint32_t>(y) * kSplashWidth * 2U);
        if (!mc_flash_read(addr, g_strip_bytes, kStripBytes)) {
            show_fallback();
            return 5U;
        }
        for (uint32_t i = 0U; i < kStripPixels; ++i) {
            const uint16_t hi = g_strip_bytes[2U * i];
            const uint16_t lo = g_strip_bytes[(2U * i) + 1U];
            g_strip_pixels[i] = kSwapBytes ? static_cast<uint16_t>((lo << 8) | hi)
                                           : static_cast<uint16_t>((hi << 8) | lo);
        }
        mc_display_write_pixels(0U, y, kSplashWidth, kStripRows, g_strip_pixels);
        mc_watchdog_feed();   // کل کار حدود یک ثانیه طول می‌کشد؛ Watchdog را تازه نگه می‌داریم
    }

    mc_display_backlight(true);
    return 1U;
}

}  // namespace mc
