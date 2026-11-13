#pragma once
// حداقل و حداکثر دما (بند ۴.۶). هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

namespace mc {

// دماها ×۱۰ هستند (2750 = 275.0 درجه)
constexpr int32_t kTempExMinX10         = 50;     // حداقل اکسترودر ( 5 درجه )
constexpr int32_t kTempExMaxX10         = 2750;   // حداکثر اکسترودر ( 275 درجه )
constexpr int32_t kTempExTargetBufferX10  = 150;  // سقف هدف = حداکثر منهای ۱۵ درجه
constexpr int32_t kTempBedMinX10        = 50;     // حداقل بستر ( 5 درجه )
constexpr int32_t kTempBedMaxX10        = 1300;   // حداکثر بستر ( 130 درجه )
constexpr int32_t kTempBedTargetBufferX10 = 100;  // سقف هدف = حداکثر منهای ۱۰ درجه

// وظیفه زمان‌بند (هر ۱۰۰ میلی‌ثانیه). اگر حد رد شود mc_kill صدا زده می‌شود.
void mc_temp_limits_poll();

}  // namespace mc
