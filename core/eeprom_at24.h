#pragma once
// درایور EEPROM نوع AT24C32D (بند ۷.۱). هیچ هدر ST اینجا include نمی‌شود.
// خواندن و نوشتن بدون انتظار است: درخواست ثبت می‌شود و وظیفه mc_eeprom_poll
// آن را در دورهای بعدی قطعه‌قطعه (حداکثر 32 بایت) انجام می‌دهد.
#include <stdint.h>
#include "../config/mc_config.h"

namespace mc {

constexpr uint8_t  kEepromI2cAddr        = MC_EEPROM_I2C_ADDR;        // آدرس I2C هفت‌بیتی
constexpr uint16_t kEepromSizeBytes      = MC_EEPROM_SIZE_BYTES;      // ظرفیت (بایت)
constexpr uint16_t kEepromPageBytes      = MC_EEPROM_PAGE_BYTES;      // اندازه صفحه (بایت)
constexpr uint32_t kEepromI2cTimeoutMs   = MC_EEPROM_I2C_TIMEOUT_MS;  // زمان انتقال I2C
constexpr uint32_t kEepromWriteTimeoutMs = MC_EEPROM_WRITE_TIMEOUT_MS; // انتظار پایان نوشتن
constexpr uint32_t kEepromPollMs         = MC_EEPROM_POLL_PERIOD_MS;  // دوره وظیفه

// وضعیت کار
enum class EepromState : uint8_t {
    Idle  = 0,   // هیچ کاری انجام نشده
    Busy  = 1,   // کار در جریان است
    Done  = 2,   // آخرین کار موفق تمام شد
    Error = 3    // آخرین کار با خطا تمام شد (کد در mc_eeprom_result)
};

// کد نتیجه آخرین کار
constexpr uint8_t kEepromResNone     = 0;  // هنوز کاری نشده
constexpr uint8_t kEepromResOk       = 1;  // موفق
constexpr uint8_t kEepromResNotFound = 2;  // تراشه جواب نداد
constexpr uint8_t kEepromResRefused  = 3;  // درخواست نامعتبر (آدرس یا طول بیرون از حافظه، طول صفر، اشاره‌گر خالی)
constexpr uint8_t kEepromResWriteFail = 4; // ارسال داده به تراشه ناموفق بود
constexpr uint8_t kEepromResReadFail  = 5; // خواندن از تراشه ناموفق بود
constexpr uint8_t kEepromResTimeout   = 6; // بعد از نوشتن، تراشه در زمان مجاز آماده نشد
constexpr uint8_t kEepromResVerify    = 7; // مقایسه بعد از خواندن (فقط فرمان آزمایشی) نخواند

// یک‌بار هنگام راه‌اندازی: فقط بررسی می‌کند تراشه جواب می‌دهد. چیزی نمی‌نویسد.
void mc_eeprom_init();
// وظیفه زمان‌بند (هر kEepromPollMs): کار در جریان را جلو می‌برد و متغیرهای debug را به‌روز می‌کند.
void mc_eeprom_poll();

// آیا تراشه در آخرین بررسی جواب داده است؟
bool mc_eeprom_present();
// خواندن len بایت از آدرس addr داخل buf. buf تا پایان کار باید معتبر بماند.
// true = درخواست پذیرفته شد (نتیجه بعداً با mc_eeprom_state و mc_eeprom_result).
// false = رد شد (نامعتبر، یا کار قبلی هنوز در جریان است).
bool mc_eeprom_read_start(uint16_t addr, uint8_t* buf, uint16_t len);
// نوشتن len بایت از data در آدرس addr. data تا پایان کار باید معتبر بماند. مقدار برگشتی مثل بالا.
bool mc_eeprom_write_start(uint16_t addr, const uint8_t* data, uint16_t len);
// وضعیت و نتیجه آخرین کار.
EepromState mc_eeprom_state();
uint8_t mc_eeprom_result();

}  // namespace mc
