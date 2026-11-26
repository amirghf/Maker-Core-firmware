#pragma once
// اجرای دستورهای حرکت G0 و G1 (بند ۵.۱۱).
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "gcode_handlers.h"

namespace mc {

// یک‌بار هنگام راه‌اندازی.
void mc_gmove_init();
// وظیفه زمان‌بند (هر ۱۰۰ میلی‌ثانیه): متغیرهای debug را به‌روز می‌کند.
void mc_gmove_poll();

// اجرای یک حرکت G0 یا G1.
GcodeStatus mc_gmove_run(const GcodeCommand& cmd, bool first_call);

// شمارنده debug: دستور M17/M18/M84 ردشده
void mc_gmove_note_mt_refused();

// ضریب سرعت M220 (درصد، ۱۰ تا ۵۰۰) (بند ۱۱.۳)
uint32_t mc_motion_speed_factor_get();
bool mc_motion_speed_factor_set(uint32_t pct);

// ضریب جریان M221 (درصد، ۱۰ تا ۲۰۰) (بند ۱۱.۳)
uint32_t mc_motion_flow_factor_get();
bool mc_motion_flow_factor_set(uint32_t pct);

}  // namespace mc
