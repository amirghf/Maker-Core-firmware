#pragma once
// ذخیره‌سازی سخت‌دار تنظیمات روی EEPROM (بند ۷.۲): دو اسلات A و B با CRC32 و شماره دنباله.
// هیچ هدر ST اینجا include نمی‌شود. همه کارها بدون انتظار است (وضعیت Busy و دورهای بعدی).
#include <stdint.h>
#include "../config/mc_config.h"

namespace mc {

constexpr uint16_t kSettingsSlotAAddr   = MC_SETTINGS_SLOT_A_ADDR;      // آدرس اسلات A
constexpr uint16_t kSettingsSlotBAddr   = MC_SETTINGS_SLOT_B_ADDR;      // آدرس اسلات B
constexpr uint16_t kSettingsSlotBytes   = MC_SETTINGS_SLOT_BYTES;       // اندازه هر اسلات
constexpr uint16_t kSettingsHeaderBytes = MC_SETTINGS_HEADER_BYTES;    // اندازه سرآیند
constexpr uint32_t kSettingsPollMs      = MC_SETTINGS_POLL_PERIOD_MS;   // دوره وظیفه
// بیشترین طول داده (payload) که در یک اسلات جا می‌شود
constexpr uint16_t kSettingsPayloadMax  = static_cast<uint16_t>(MC_SETTINGS_SLOT_BYTES - MC_SETTINGS_HEADER_BYTES);

// وضعیت کار
enum class SettingsState : uint8_t {
    Idle  = 0,   // هیچ کاری انجام نشده
    Busy  = 1,   // کار در جریان است
    Done  = 2,   // آخرین کار موفق تمام شد
    Error = 3    // آخرین کار با خطا تمام شد (کد در mc_settings_result)
};

// کد نتیجه آخرین کار
constexpr uint8_t kSetResNone       = 0;  // هنوز کاری نشده
constexpr uint8_t kSetResOk         = 1;  // موفق
constexpr uint8_t kSetResEepromFail = 2;  // EEPROM جواب نداد، خطا داد یا زمان مرحله تمام شد
constexpr uint8_t kSetResNoValid    = 3;  // (فقط خواندن) هیچ‌کدام از دو اسلات سالم نبود
constexpr uint8_t kSetResVerifyFail = 4;  // بعد از نوشتن، داده خوانده‌شده با داده نوشته‌شده نخواند
constexpr uint8_t kSetResRefused    = 5;  // درخواست نامعتبر (اشاره‌گر خالی، طول صفر، طول بیش از حد، بافر کوچک)

// یک‌بار هنگام راه‌اندازی. فقط خودآزمایی CRC انجام می‌دهد؛ به EEPROM دست نمی‌زند.
void mc_settings_init();
// وظیفه زمان‌بند (هر kSettingsPollMs).
void mc_settings_poll();

// ذخیره: payload (len بایت) در اسلات قدیمی‌تر یا خراب نوشته می‌شود و بعد verify می‌شود.
// payload همان لحظه کپی می‌شود و بعد از برگشت لازم نیست معتبر بماند.
// true = درخواست پذیرفته شد؛ false = رد شد (نامعتبر یا کار قبلی در جریان است).
bool mc_settings_save_start(const uint8_t* payload, uint16_t len);
// بازیابی: داده اسلات سالم‌تر (با seq بزرگ‌تر) در out کپی می‌شود (حداکثر max_len بایت).
// out تا پایان کار باید معتبر بماند. طول واقعی بعد از پایان با mc_settings_loaded_len خوانده می‌شود.
bool mc_settings_load_start(uint8_t* out, uint16_t max_len);

SettingsState mc_settings_state();
uint8_t       mc_settings_result();
uint16_t      mc_settings_loaded_len();

}  // namespace mc
