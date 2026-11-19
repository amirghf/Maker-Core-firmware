#pragma once
// هومینگ (بند ۶.۳ و ۶.۷): دستور G28 دو مرحله‌ای برای X و Y، و هومینگ ایمن Z با BLTouch و نقطه امن (100, 60).
// جهت هومینگ همه محورها منفی است و بعد از هومینگ موقعیت X و Y صفر و Z برابر منفی آفست Z پراب می‌شود.
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "gcode_handlers.h"

namespace mc {

// یک‌بار هنگام راه‌اندازی.
void mc_homing_init();
// وظیفه زمان‌بند (هر ۱۰۰ میلی‌ثانیه): متغیرهای debug را به‌روز می‌کند.
void mc_homing_poll();
// اجرای G28. تا پایان کار Busy برمی‌گرداند و منتظر نمی‌ماند.
GcodeStatus mc_homing_run(const GcodeCommand& cmd, bool first_call);

}  // namespace mc
