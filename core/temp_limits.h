#pragma once
// حداقل و حداکثر دما (بند ۴.۶). هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "../config/mc_config.h"

namespace mc {

// دماها ×۱۰ هستند (از config/mc_config.h)
constexpr int32_t kTempExMinX10         = MC_HOTEND_MIN_TEMP_C * 10;
constexpr int32_t kTempExMaxX10         = MC_HOTEND_MAX_TEMP_C * 10;
constexpr int32_t kTempExTargetBufferX10  = MC_HOTEND_TARGET_BUFFER_C * 10;
constexpr int32_t kTempBedMinX10        = MC_BED_MIN_TEMP_C * 10;
constexpr int32_t kTempBedMaxX10        = MC_BED_MAX_TEMP_C * 10;
constexpr int32_t kTempBedTargetBufferX10 = MC_BED_TARGET_BUFFER_C * 10;

// وظیفه زمان‌بند (هر ۱۰۰ میلی‌ثانیه). اگر حد رد شود mc_kill صدا زده می‌شود.
void mc_temp_limits_poll();

}  // namespace mc
