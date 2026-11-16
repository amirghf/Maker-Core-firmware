#pragma once
// اکسترودر (بند ۵.۹): جلوگیری از اکستروژن سرد و دستور M302.
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

namespace mc {

constexpr int32_t kExDefaultMinTempX10 = 1700;   // حداقل دمای اکستروژن پیش‌فرض ×۱۰ (170 درجه)
constexpr int32_t kExMaxMinTempDeg     = 275;    // بیشترین مقدار مجاز S در M302 (درجه)

// یک‌بار هنگام راه‌اندازی.
void mc_extruder_init();
// وظیفه زمان‌بند (هر ۱۰۰ میلی‌ثانیه): متغیرهای debug را به‌روز می‌کند.
void mc_extruder_poll();

// آیا اکستروژن (حرکت E) الان مجاز است؟ (قاعده در توضیح بالای این بند آمده)
bool mc_extruder_allowed();

// حداقل دمای اکستروژن ×۱۰. false اگر بیرون از بازه ۰ تا ۲۷۵ درجه (۰ تا ۲۷۵۰ ×۱۰) باشد؛ در آن حالت چیزی عوض نمی‌شود.
int32_t mc_extruder_min_temp_x10_get();
bool mc_extruder_min_temp_x10_set(int32_t t_x10);

// اکستروژن سرد: true = مجاز (جلوگیری خاموش). M302 P1 ← true ، M302 P0 ← false
bool mc_extruder_cold_ok_get();
void mc_extruder_cold_ok_set(bool on);

// شمارنده debug: بلوکی که به‌خاطر اکستروژن سرد رد شد
void mc_extruder_note_refused();

}  // namespace mc
