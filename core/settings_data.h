#pragma once
// جمع‌آوری، بررسی و اعمال تنظیمات فرمویر (بند ۷.۳).
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "settings_store.h"

namespace mc {

// تنظیمات فعلی را در out می‌نویسد و طول را برمی‌گرداند (0 یعنی نشد)
uint16_t mc_cfg_collect(uint8_t* out, uint16_t max_len);

// 0 یعنی معتبر؛ 4 اندازه یا نسخه نادرست؛ 5 مقدار نامعتبر
uint8_t mc_cfg_check(const uint8_t* in, uint16_t len);

// فقط بعد از check موفق صدا زده شود. تنظیمات را در RAM اعمال می‌کند.
bool mc_cfg_apply(const uint8_t* in, uint16_t len);

// همه تنظیمات را به مقادیر پیش‌فرض برمی‌گرداند.
void mc_cfg_apply_defaults();

// ثبت وضعیت دیباگ دستورهای M500 تا M503
void mc_cfg_note(uint32_t cmd, uint8_t result, uint16_t len, uint8_t store_res);

}  // namespace mc
