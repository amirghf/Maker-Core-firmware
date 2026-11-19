#pragma once
// حفاظت‌های watch heating، thermal runaway و heater failed (بند ۴.۷). هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "../config/mc_config.h"

namespace mc {

// دماها ×۱۰ و زمان‌ها ثانیه هستند (از config/mc_config.h).
constexpr uint32_t kExWatchPeriodS     = MC_HOTEND_WATCH_PERIOD_S;    // اکسترودر: دوره watch heating
constexpr int32_t  kExWatchRiseX10     = MC_HOTEND_WATCH_RISE_C * 10;  // اکسترودر: افزایش لازم در هر پنجره
constexpr int32_t  kExRunawayHystX10   = MC_HOTEND_RUNAWAY_HYST_C * 10; // اکسترودر: هیسترزیس runaway
constexpr uint32_t kExRunawayPeriodS   = MC_HOTEND_RUNAWAY_PERIOD_S;  // اکسترودر: دوره thermal runaway
constexpr uint32_t kExFailedPeriodS    = MC_HOTEND_FAILED_PERIOD_S;   // اکسترودر: حداکثر زمان رسیدن به هدف

constexpr uint32_t kBedWatchPeriodS    = MC_BED_WATCH_PERIOD_S;   // بستر: دوره watch heating
constexpr int32_t  kBedWatchRiseX10    = MC_BED_WATCH_RISE_C * 10; // بستر: افزایش لازم در هر پنجره
constexpr int32_t  kBedRunawayHystX10  = MC_BED_RUNAWAY_HYST_C * 10; // بستر: هیسترزیس runaway
constexpr uint32_t kBedRunawayPeriodS  = MC_BED_RUNAWAY_PERIOD_S; // بستر: دوره thermal runaway
constexpr uint32_t kBedFailedPeriodS   = MC_BED_FAILED_PERIOD_S;  // بستر: حداکثر زمان رسیدن به هدف

// وظیفه زمان‌بند (هر ۱۰۰ میلی‌ثانیه). اگر شرطی رد شود mc_kill صدا زده می‌شود.
void mc_temp_protect_poll();

}  // namespace mc
