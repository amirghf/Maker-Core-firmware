#pragma once
// ذخیره و بارگذاری ضرایب کالیبراسیون لمس روی EEPROM (بند ۸.۱۵).
// کالیبراسیون لمس مربوط به سخت‌افزار برد و نمایشگر است، نه تنظیمات چاپ؛
// بنابراین M500 و M502 و بازگشت به کارخانه این رکورد را نمی‌خوانند، نمی‌نویسند و پاک نمی‌کنند.
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "../config/mc_config.h"

namespace mc {

constexpr uint16_t kTouchCalAddr = MC_TOUCH_CAL_ADDR; // آدرس ۵۱۲

void mc_tcal_store_init();
void mc_tcal_store_poll();   // ماشین حالت غیرمسدودکننده؛ هر دور فقط یک قدم

// درخواست خواندن و اعمال ضرایب (با mc_touch_set_coef اگر CRC و magic و نسخه درست بود)
bool mc_tcal_store_load_start();

// ذخیره ضرایب فعلی (mc_touch_get_coef)
bool mc_tcal_store_save_start();

// رکورد را نامعتبر می‌کند (magic را صفر می‌نویسد)
bool mc_tcal_store_clear_start();

// وضعیت ماژول ذخیره: 0 بیکار، 1 در جریان، 2 موفق تمام شد، 3 خطا
uint8_t mc_tcal_store_state();

// کد نتیجه: 0 هنوز کاری نشده، 1 موفق، 2 EEPROM جواب نداد یا خطا، 3 رکورد معتبر نیست، 4 verify ناموفق، 5 درخواست نامعتبر
uint8_t mc_tcal_store_result();

}  // namespace mc
