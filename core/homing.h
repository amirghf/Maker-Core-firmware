#pragma once
// هومینگ (بند ۶.۳): دستور G28 دو مرحله‌ای (سریع و آهسته) برای محورهای X و Y و Z.
// جهت هومینگ همه محورها منفی است و بعد از هومینگ موقعیت محور برابر صفر می‌شود.
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
