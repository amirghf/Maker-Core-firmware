#pragma once
// وضعیت ماشین که دستورها می‌خوانند: موقعیت محورها و «آیا حرکت در جریان است».
// فعلاً مقدارها ثابت‌اند؛ مرحله ۵ (حرکت) آن‌ها را واقعی می‌کند.
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

namespace mc {

constexpr uint8_t kAxisX = 0;
constexpr uint8_t kAxisY = 1;
constexpr uint8_t kAxisZ = 2;
constexpr uint8_t kAxisE = 3;
constexpr uint8_t kAxisCount = 4;

// موقعیت فعلی یک محور به میلی‌متر (شماره محور خارج از محدوده ← ۰).
float mc_position_get(uint8_t axis);
// موقعیت یک محور را می‌گذارد (شماره محور خارج از محدوده نادیده گرفته می‌شود).
void mc_position_set(uint8_t axis, float mm);

// آیا حرکتی در جریان یا در صف است؟ (فعلاً فقط از متغیر تست mc_dbg_motion_fake_busy خوانده می‌شود)
bool mc_motion_busy();

}  // namespace mc
