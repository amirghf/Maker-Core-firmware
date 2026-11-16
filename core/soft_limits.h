#pragma once
// حدود نرم‌افزاری (بند ۵.۱۰): حداقل و حداکثر موقعیت هر محور X و Y و Z به میلی‌متر.
// محور E حد ندارد. این ماژول فقط عدد نگه می‌دارد و بررسی می‌کند؛ به سخت‌افزار دست نمی‌زند.
// بند ۵.۱۱ (G0 و G1) از mc_softlimit_allows و mc_softlimit_allows_all استفاده می‌کند.
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

namespace mc {

// مقدارهای پیش‌فرض (اندازه مسیر قابل استفاده دستگاه؛ بعد از هومینگ در مرحله ۶ دقیق می‌شوند)
constexpr float kSoftLimitDefaultMinMm  = 0.0f;
constexpr float kSoftLimitDefaultMaxXMm = 240.0f;
constexpr float kSoftLimitDefaultMaxYMm = 120.0f;
constexpr float kSoftLimitDefaultMaxZMm = 200.0f;

// یک‌بار هنگام راه‌اندازی.
void mc_softlimit_init();
// وظیفه زمان‌بند (هر ۱۰۰ میلی‌ثانیه): درخواست‌های متغیرهای debug را اجرا و متغیرها را به‌روز می‌کند.
void mc_softlimit_poll();

// کلید فعال یا غیرفعال بودن همه حدها (پیش‌فرض: فعال).
bool mc_softlimit_enabled_get();
void mc_softlimit_enabled_set(bool enabled);

// حداقل و حداکثر یک محور (۰ = X ، ۱ = Y ، ۲ = Z). شماره بد ← ۰.
float mc_softlimit_min_get(uint8_t axis);
float mc_softlimit_max_get(uint8_t axis);
// false اگر شماره محور بد است (فقط ۰ تا ۲)، یا مقدار NaN یا بیرون از ±kModeMaxPositionMm است،
// یا حداقل بزرگ‌تر یا مساوی حداکثر است؛ در آن حالت چیزی عوض نمی‌شود.
bool mc_softlimit_set(uint8_t axis, float min_mm, float max_mm);

// آیا این مقصد برای این محور مجاز است؟
// true اگر حدها غیرفعال‌اند، یا محور E (یا شماره‌ای غیر از ۰ تا ۲) است، یا حداقل ≤ مقصد ≤ حداکثر.
// false اگر حدها فعال‌اند و مقصد بیرون از حد یا NaN است.
bool mc_softlimit_allows(uint8_t axis, float target_mm);
// مقصد هر چهار محور (X، Y، Z، E) را می‌گیرد. true فقط اگر X و Y و Z هر سه مجاز باشند.
bool mc_softlimit_allows_all(const float target_mm[4]);

// شمارنده debug: هر رد شدن را بشمار (هم نوشتن نامعتبر و هم مقصد نامجاز).
void mc_softlimit_note_refused();

}  // namespace mc
