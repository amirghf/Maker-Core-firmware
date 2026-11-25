#pragma once
// حدود نرم‌افزاری (بند ۵.۱۰): حداقل و حداکثر موقعیت هر محور X و Y و Z به میلی‌متر.
// محور E حد ندارد. این ماژول فقط عدد نگه می‌دارد و بررسی می‌کند؛ به سخت‌افزار دست نمی‌زند.
// بند ۵.۱۱ (G0 و G1) از mc_softlimit_allows و mc_softlimit_allows_all استفاده می‌کند.
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "../config/mc_config.h"

namespace mc {

// مقدارهای پیش‌فرض (اندازه مسیر قابل استفاده دستگاه؛ بعد از هومینگ در مرحله ۶ دقیق می‌شوند)
constexpr float kSoftLimitDefaultMinMm  = MC_SOFT_MIN_MM;
constexpr float kSoftLimitDefaultMaxXMm = MC_SOFT_MAX_X_MM;
constexpr float kSoftLimitDefaultMaxYMm = MC_SOFT_MAX_Y_MM;
constexpr float kSoftLimitDefaultMaxZMm = MC_SOFT_MAX_Z_MM;

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
// بند ۱۰.۶: ثبت رد شدن به‌خاطر خروج از محدوده بعد از هومینگ (محور ۰=X، ۱=Y، ۲=Z، ۳=G92)
void mc_softlimit_note_homed_refused(uint8_t axis);

}  // namespace mc
