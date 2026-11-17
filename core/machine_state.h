#pragma once
// وضعیت ماشین که دستورها می‌خوانند: موقعیت محورها و «آیا حرکت در جریان است».
// بند ۵.۱۲: mc_motion_busy به حرکت واقعی وصل است.
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

// آیا حرکتی در جریان یا در صف است؟ true اگر بلوک حرکت در صف یا در اجراست (mc_mq_busy) یا متغیر تست mc_dbg_185_motion_fake_busy روشن است.
bool mc_motion_busy();

}  // namespace mc
