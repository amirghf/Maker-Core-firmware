#pragma once
// ذخیره و بارگذاری تنظیمات نمایشگر (زمان خاموشی و زمان خروج خودکار از منو) روی EEPROM (بند ۸.۲۱).
// این رکورد مربوط به رابط کاربری است، نه تنظیمات چاپ؛
// بنابراین M500 و M502 و بازگشت به کارخانه آن را نمی‌خوانند، نمی‌نویسند و پاک نمی‌کنند.
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "../config/mc_config.h"

namespace mc {

constexpr uint16_t kDispSetAddr = MC_DISPSET_ADDR;   // آدرس رکورد در EEPROM

// مقدار پیش‌فرض را از ماکروها می‌گذارد. به EEPROM دست نمی‌زند.
void mc_dispset_init();
// ماشین حالت غیرمسدودکننده؛ هر دور فقط یک قدم (از زمان‌بند صدا زده می‌شود).
void mc_dispset_poll();

// مقدارهای فعلی در RAM (ثانیه). 0 = هیچ‌وقت.
uint32_t mc_dispset_screen_off_s();
uint32_t mc_dispset_menu_exit_s();

// مقدارها را فقط در RAM عوض می‌کند. اگر یکی از دو مقدار بیشتر از MC_UI_DISP_MAX_S باشد false برمی‌گرداند و چیزی عوض نمی‌شود.
bool mc_dispset_set_values(uint32_t screen_off_s, uint32_t menu_exit_s);

// خواندن رکورد از EEPROM و اعمال آن (اگر رکورد سالم بود).
bool mc_dispset_load_start();
// ذخیره مقدارهای فعلی در EEPROM (با verify).
bool mc_dispset_save_start();
// رکورد را نامعتبر می‌کند (صفر نوشتن)؛ مقدارهای RAM عوض نمی‌شوند.
bool mc_dispset_clear_start();

// وضعیت: 0 بیکار، 1 در جریان، 2 موفق تمام شد، 3 خطا
uint8_t mc_dispset_state();
// کد نتیجه: 0 هنوز کاری نشده، 1 موفق، 2 EEPROM جواب نداد یا زمان تمام شد، 3 رکورد معتبر نیست (یا هنوز ذخیره نشده)، 4 verify ناموفق
uint8_t mc_dispset_result();

}  // namespace mc
