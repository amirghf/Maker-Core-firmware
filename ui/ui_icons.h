#pragma once
// آیکون‌ها و ابزارهای ترسیم کارت و نوار پیشرفت رابط کاربری (بند ۸.۷).
// همه با ابزار گرافیکی gfx کشیده می‌شوند (بدون عکس و بدون float).
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

namespace mc {

// دایره پر با مرکز (cx, cy) و شعاع r. فقط با عدد صحیح محاسبه می‌شود.
void mc_ui_fill_circle(uint16_t cx, uint16_t cy, uint16_t r, uint16_t color);

// مستطیل پر با گوشه گرد شعاع r. محاسبه فقط با عدد صحیح.
void mc_ui_fill_rrect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color);

// کارت یا دکمه گوشه‌گرد با حاشیه border و رنگ داخل fill.
void mc_ui_card(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t fill, uint16_t border);

// مثلث پر بین سه نقطه با محاسبه edge function به صورت عدد صحیح int32_t.
void mc_ui_fill_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);

// نوار پیشرفت افقی: بخش پر با fill_color و بقیه با track_color.
void mc_ui_progress(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t fill_px, uint16_t fill_color, uint16_t track_color);

// آیکون جهت حرکت موتور (جعبه ۱۲ در ۱۲). positive = true (مثلث رو به بالا)، positive = false (مثلث رو به پایین).
void mc_icon_dir(uint16_t x, uint16_t y, bool positive, uint16_t color);

// آیکون‌های کوچک (۲۴ در ۲۴). پارامتر bg رنگ پس‌زمینه است و برای بریدگی‌ها استفاده می‌شود.
void mc_icon_sd(uint16_t x, uint16_t y, uint16_t color, uint16_t bg);
void mc_icon_usb(uint16_t x, uint16_t y, uint16_t color, uint16_t bg);
void mc_icon_menu(uint16_t x, uint16_t y, uint16_t color);

// آیکون‌های ۳۲ در ۳۲ دورنگ برای کارت‌های هیتر و فن
void mc_icon_nozzle(uint16_t x, uint16_t y, uint16_t accent, uint16_t body);
void mc_icon_fan(uint16_t x, uint16_t y, uint16_t accent, uint16_t body, uint16_t bg);
void mc_icon_bed(uint16_t x, uint16_t y, uint16_t accent, uint16_t body);

// آیکون‌های دکمه (۳۲ در ۳۲)
void mc_icon_play(uint16_t x, uint16_t y, uint16_t color);
void mc_icon_pause(uint16_t x, uint16_t y, uint16_t color);
void mc_icon_stop(uint16_t x, uint16_t y, uint16_t color);
void mc_icon_reset(uint16_t x, uint16_t y, uint16_t color);

}  // namespace mc
