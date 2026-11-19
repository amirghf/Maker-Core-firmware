#pragma once
// رابط سروو مستقل از برد (بند ۶.۵): پالس ۵۰ هرتزی با عرض قابل تنظیم. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

namespace mc {

// بازه مجاز عرض پالس (میکروثانیه)
constexpr uint32_t kServoMinPulseUs = 400U;
constexpr uint32_t kServoMaxPulseUs = 2600U;

// پالس سروو را با عرض pulse_us (بین kServoMinPulseUs و kServoMaxPulseUs) و دوره ۲۰ میلی‌ثانیه شروع می‌کند.
// عرض نامعتبر = false و هیچ پالسی داده نمی‌شود. اگر از قبل پالس در جریان باشد، فقط عرض عوض می‌شود.
bool mc_servo_start(uint32_t pulse_us);
// پالس را قطع می‌کند و پین سروو را در حالت امن (خروجی با سطح LOW) می‌گذارد. اگر پالسی نباشد بی‌اثر است.
void mc_servo_stop();
// آیا الان پالس داده می‌شود؟
bool mc_servo_active();
// عرض پالس فعلی (میکروثانیه)؛ اگر پالسی نباشد ۰.
uint32_t mc_servo_pulse_us();
// متغیرهای debug لایه سروو را به‌روز می‌کند (از ماژول BLTouch صدا زده می‌شود).
void mc_servo_debug_poll();

}  // namespace mc
