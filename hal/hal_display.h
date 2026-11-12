#pragma once
// رابط نمایشگر مستقل از برد. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
namespace mc {

// چند رنگ آماده در فرمت RGB565 (۱۶ بیتی)
constexpr uint16_t kColorBlack = 0x0000U;
constexpr uint16_t kColorRed   = 0xF800U;
constexpr uint16_t kColorGreen = 0x07E0U;
constexpr uint16_t kColorBlue  = 0x001FU;
constexpr uint16_t kColorWhite = 0xFFFFU;

// نمایشگر را راه‌اندازی می‌کند (ریست و دنباله راه‌اندازی). بک‌لایت روشن نمی‌شود.
// true = همه ارسال‌ها به نمایشگر موفق بود.
bool mc_display_init();
// بک‌لایت را روشن یا خاموش می‌کند.
void mc_display_backlight(bool on);
// ابعاد صفحه در حالت افقی (پیکسل).
uint16_t mc_display_width();
uint16_t mc_display_height();
// مستطیل را با یک رنگ پر می‌کند. بخش بیرون از صفحه بریده می‌شود. قبل از mc_display_init کاری نمی‌کند.
void mc_display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
// کل صفحه را با یک رنگ پر می‌کند.
void mc_display_fill_screen(uint16_t color);
// یک مستطیل از پیکسل‌های آماده را روی صفحه می‌نویسد. pixels باید w*h عدد RGB565 باشد (سطر به سطر).
// بخش بیرون از صفحه بریده نمی‌شود؛ فراخواننده باید مطمئن باشد مستطیل داخل صفحه است. قبل از mc_display_init کاری نمی‌کند.
void mc_display_write_pixels(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* pixels);

}  // namespace mc
#endif
