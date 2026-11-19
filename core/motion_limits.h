#pragma once
// حدود سرعت و شتاب حرکت (بند ۵.۱۱): M203، M204 و M205.
// این ماژول فقط عدد نگه می‌دارد و به سخت‌افزار دست نمی‌زند.
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "../config/mc_config.h"

namespace mc {

// مقادیر پیش‌فرض (تا بند ۷ که ذخیره‌سازی در EEPROM می‌آید موقت‌اند)
constexpr float kMlimDefaultFeedXMmS = MC_MAX_FEED_X_MM_S;
constexpr float kMlimDefaultFeedYMmS = MC_MAX_FEED_Y_MM_S;
constexpr float kMlimDefaultFeedZMmS = MC_MAX_FEED_Z_MM_S;
constexpr float kMlimDefaultFeedEMmS = MC_MAX_FEED_E_MM_S;

constexpr float kMlimMinFeedMmS = 0.1f;
constexpr float kMlimMaxFeedMmS = 1000.0f;

constexpr float kMlimDefaultAccPrint   = MC_ACCEL_PRINT_MM_S2;
constexpr float kMlimDefaultAccRetract = MC_ACCEL_RETRACT_MM_S2;
constexpr float kMlimDefaultAccTravel  = MC_ACCEL_TRAVEL_MM_S2;

constexpr float kMlimMinAccMmS2 = 1.0f;
constexpr float kMlimMaxAccMmS2 = 10000.0f;

constexpr float kMlimDefaultMinFeedMmS = MC_MIN_FEED_MM_S;
constexpr float kMlimMinMinFeedMmS     = 0.1f;
constexpr float kMlimMaxMinFeedMmS     = 100.0f;

// یک‌بار هنگام راه‌اندازی.
void mc_mlim_init();
// وظیفه زمان‌بند (هر ۱۰۰ میلی‌ثانیه): متغیرهای debug را به‌روز می‌کند.
void mc_mlim_poll();

// حداکثر سرعت هر محور (mm/s): axis: 0=X, 1=Y, 2=Z, 3=E. شماره بد ← 1.0f
float mc_mlim_feed_get(uint8_t axis);
// false اگر شماره محور بد است یا مقدار بیرون از بازه [0.1, 1000] یا NaN است؛ در آن حالت چیزی عوض نمی‌شود.
bool mc_mlim_feed_set(uint8_t axis, float mm_s);

// شتاب هر نوع (mm/s²): kind: 0=print, 1=retract, 2=travel. نوع بد ← 100.0f
float mc_mlim_acc_get(uint8_t kind);
// false اگر نوع بد است یا مقدار بیرون از بازه [1, 10000] یا NaN است؛ در آن حالت چیزی عوض نمی‌شود.
bool mc_mlim_acc_set(uint8_t kind, float mm_s2);

// حداقل سرعت شروع و پایان (mm/s)
float mc_mlim_minfeed_get();
// false اگر مقدار بیرون از بازه [0.1, 100] یا NaN است؛ در آن حالت چیزی عوض نمی‌شود.
bool mc_mlim_minfeed_set(float mm_s);

// شمارنده debug: دستورهای ردشده
void mc_mlim_note_refused();

}  // namespace mc
