#pragma once
// آیکون‌های رابط کاربری (بند ۸.۷). همه با ابزار گرافیکی gfx کشیده می‌شوند (بدون عکس).
// هیچ هدر ST اینجا include نمی‌شود.
// x و y گوشه بالا-چپ جعبه آیکون هستند. آیکون‌های کوچک جعبه ۲۴ در ۲۴ و آیکون‌های دکمه (play، pause، stop، reset) جعبه ۳۲ در ۳۲ دارند.
#include <stdint.h>

namespace mc {

// دایره پر با مرکز (cx, cy) و شعاع r. فقط با عدد صحیح محاسبه می‌شود.
void mc_ui_fill_circle(uint16_t cx, uint16_t cy, uint16_t r, uint16_t color);

// آیکون‌های کوچک (۲۴ در ۲۴). پارامتر bg رنگ پس‌زمینه است و برای سوراخ‌ها و بریدگی‌ها استفاده می‌شود.
void mc_icon_sd(uint16_t x, uint16_t y, uint16_t color, uint16_t bg);
void mc_icon_usb(uint16_t x, uint16_t y, uint16_t color, uint16_t bg);
void mc_icon_menu(uint16_t x, uint16_t y, uint16_t color);
void mc_icon_nozzle(uint16_t x, uint16_t y, uint16_t color);
void mc_icon_fan(uint16_t x, uint16_t y, uint16_t color, uint16_t bg);
void mc_icon_bed(uint16_t x, uint16_t y, uint16_t color);

// آیکون‌های دکمه (۳۲ در ۳۲)
void mc_icon_play(uint16_t x, uint16_t y, uint16_t color);
void mc_icon_pause(uint16_t x, uint16_t y, uint16_t color);
void mc_icon_stop(uint16_t x, uint16_t y, uint16_t color);
void mc_icon_reset(uint16_t x, uint16_t y, uint16_t color);

}  // namespace mc
