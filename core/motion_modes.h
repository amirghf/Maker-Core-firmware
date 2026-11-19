#pragma once
// حالت‌ها و واحدهای حرکت (بند ۵.۸): حالت مطلق/نسبی، حالت E، گام بر میلی‌متر (M92) و سرعت پیشروی (فید).
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "../config/mc_config.h"

namespace mc {

constexpr float kModeDefaultStepsPerMm[4] = { MC_STEPS_PER_MM_X, MC_STEPS_PER_MM_Y, MC_STEPS_PER_MM_Z, MC_STEPS_PER_MM_E }; // گام بر میلی‌متر پیش‌فرض هر محور (بند ۶.۷، تا بند ۷ که ذخیره‌سازی می‌آید)
constexpr float kModeMinStepsPerMm     = 1.0f;      // کمترین مقدار مجاز M92
constexpr float kModeMaxStepsPerMm     = 10000.0f;  // بیشترین مقدار مجاز M92
constexpr float kModeDefaultFeedMmMin  = MC_DEFAULT_FEED_MM_MIN; // فید پیش‌فرض (میلی‌متر بر دقیقه)
constexpr float kModeMinFeedMmMin      = 1.0f;      // کمترین فید مجاز
constexpr float kModeMaxFeedMmMin      = 60000.0f;  // بیشترین فید مجاز
constexpr float kModeMaxPositionMm     = 99999.0f;  // بیشترین قدر مطلق موقعیت مجاز در G92

// یک‌بار هنگام راه‌اندازی.
void mc_mode_init();
// وظیفه زمان‌بند (هر ۱۰۰ میلی‌ثانیه): متغیرهای debug را به‌روز می‌کند و مسیر تست فید را اجرا می‌کند.
void mc_mode_poll();

// G90 ← false ، G91 ← true
bool mc_mode_relative_get();
void mc_mode_relative_set(bool relative);
// M82 ← false ، M83 ← true. نتیجه مؤثر E نسبی است اگر G91 یا M83 فعال باشد.
bool mc_mode_e_relative_get();
void mc_mode_e_relative_set(bool relative);

// گام بر میلی‌متر هر محور (۰ = X ، ۱ = Y ، ۲ = Z ، ۳ = E). شماره بد ← مقدار پیش‌فرض.
float mc_mode_spmm_get(uint8_t axis);
// false اگر شماره محور بد است یا مقدار بیرون از بازه (یا NaN) است؛ در آن حالت چیزی عوض نمی‌شود.
bool mc_mode_spmm_set(uint8_t axis, float value);

// فید فعلی. mc_mode_feed_get_mms همان فید بر حسب میلی‌متر بر ثانیه است (فید ÷ ۶۰).
float mc_mode_feed_get_mmin();
float mc_mode_feed_get_mms();
// false اگر مقدار بیرون از بازه (یا NaN) است؛ در آن حالت چیزی عوض نمی‌شود.
bool mc_mode_feed_set(float mm_per_min);

// شمارنده‌های debug: دستور پذیرفته‌شده / ردشده
void mc_mode_note_applied();
void mc_mode_note_refused();

}  // namespace mc
