#pragma once
// رابط Watchdog مستقل از برد. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
namespace mc {

// Watchdog را با زمان تایم‌اوت (میلی‌ثانیه، بین ۱۰۰ تا ۸۰۰۰) روشن می‌کند.
// بعد از روشن‌شدن دیگر خاموش نمی‌شود. علت ریست قبلی را هم همین‌جا ثبت می‌کند.
void mc_watchdog_start(uint32_t timeout_ms);
// شمارنده Watchdog را تازه می‌کند. اگر Watchdog روشن نباشد کاری نمی‌کند.
void mc_watchdog_feed();
// آیا Watchdog روشن است؟
bool mc_watchdog_active();
// آیا آخرین ریست برد به‌خاطر Watchdog بود؟ (بعد از mc_watchdog_start معتبر است)
bool mc_watchdog_was_reset();

}  // namespace mc

extern "C" {
#endif

// پل برای کد C (main.c)
void mc_watchdog_start_c(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
