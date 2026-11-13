#pragma once
// حفاظت‌های watch heating، thermal runaway و heater failed (بند ۴.۷). هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

namespace mc {

// دماها ×۱۰ و زمان‌ها ثانیه هستند.
constexpr uint32_t kExWatchPeriodS     = 20U;    // اکسترودر: دوره watch heating
constexpr int32_t  kExWatchRiseX10     = 20;     // اکسترودر: افزایش لازم در هر پنجره ( 2 درجه )
constexpr int32_t  kExRunawayHystX10   = 40;     // اکسترودر: هیسترزیس runaway ( 4 درجه )
constexpr uint32_t kExRunawayPeriodS   = 40U;    // اکسترودر: دوره thermal runaway
constexpr uint32_t kExFailedPeriodS    = 600U;   // اکسترودر: حداکثر زمان رسیدن به هدف

constexpr uint32_t kBedWatchPeriodS    = 60U;    // بستر: دوره watch heating
constexpr int32_t  kBedWatchRiseX10    = 20;     // بستر: افزایش لازم در هر پنجره ( 2 درجه )
constexpr int32_t  kBedRunawayHystX10  = 40;     // بستر: هیسترزیس runaway ( 4 درجه )
constexpr uint32_t kBedRunawayPeriodS  = 60U;    // بستر: دوره thermal runaway
constexpr uint32_t kBedFailedPeriodS   = 1800U;  // بستر: حداکثر زمان رسیدن به هدف

// وظیفه زمان‌بند (هر ۱۰۰ میلی‌ثانیه). اگر شرطی رد شود mc_kill صدا زده می‌شود.
void mc_temp_protect_poll();

}  // namespace mc
