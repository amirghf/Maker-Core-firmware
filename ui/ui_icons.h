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

// آیکون جفت‌مثلث جهت موتور بدون دم (جعبه ۱۴ در ۲۲). active: 0 = هر دو کم‌رنگ، 1 = مثلث بالا (مثبت)، 2 = مثلث پایین (منفی) (بند ۸.۷).
void mc_icon_dir_pair(uint16_t x, uint16_t y, uint8_t active, uint16_t active_color, uint16_t dim_color);

// جفت‌مثلث عمودی (جعبه ۱۴ در ۲۲): mode: 0 = هر دو کم‌رنگ، 1 = مثلث بالا روشن، 2 = مثلث پایین روشن، 3 = هر دو روشن.
void mc_icon_dir_pair_v(uint16_t x, uint16_t y, uint8_t mode, uint16_t top_color, uint16_t bottom_color, uint16_t dim_color);

// جفت‌مثلث افقی (جعبه ۲۲ در ۱۴): mode: 0 = هر دو کم‌رنگ، 1 = مثلث راست روشن، 2 = مثلث چپ روشن، 3 = هر دو روشن.
void mc_icon_dir_pair_h(uint16_t x, uint16_t y, uint8_t mode, uint16_t right_color, uint16_t left_color, uint16_t dim_color);

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

// آیکون آیتم منو در جعبه ۳۲ در ۳۲ (بند ۸.۷). id = ۰ تا ۸.
void mc_icon_menu_item(uint16_t x, uint16_t y, uint8_t id, uint16_t color, uint16_t bg);

}  // namespace mc
