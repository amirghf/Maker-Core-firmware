#pragma once
// رابط زمان مستقل از برد. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
namespace mc {

// زمان‌پایه را راه‌اندازی می‌کند (تایمر سخت‌افزاری با وقفه هر ۱۰۰ میکروثانیه).
// یک‌بار بعد از راه‌اندازی برد صدا زده شود.
void mc_time_init();
// میلی‌ثانیه از روشن‌شدن (۳۲ بیتی؛ بعد از حدود ۴۹ روز از صفر شروع می‌شود).
uint32_t mc_millis();
// میکروثانیه (۳۲ بیتی؛ هر حدود ۷۱ دقیقه از صفر شروع می‌شود).
// برای اندازه‌گیری فاصله همیشه از تفریق بدون‌علامت استفاده کن: mc_micros() - start
uint32_t mc_micros();
// تعداد تیک‌های ۱۰۰ میکروثانیه‌ای از شروع زمان‌پایه.
uint32_t mc_time_ticks();
// تست خودکار زمان‌پایه (حدود ۱۰۰ میلی‌ثانیه طول می‌کشد). true = سالم.
bool mc_time_selftest();

// آیا از زمان start_ms حداقل interval_ms میلی‌ثانیه گذشته؟ (با دور زدن شمارنده هم درست کار می‌کند)
inline bool mc_time_elapsed_ms(uint32_t start_ms, uint32_t interval_ms) {
    return (mc_millis() - start_ms) >= interval_ms;
}

}  // namespace mc

extern "C" {
#endif

// پل برای کد C (main.c)
void mc_time_init_c(void);
int  mc_time_selftest_c(void);      // نتیجه را در mc_time_selftest_ok هم می‌گذارد
void mc_time_tick_c(void);          // فقط از وقفه تایمر زمان‌پایه صدا زده شود
void mc_time_debug_poll_c(void);    // متغیرهای دیباگ را به‌روز می‌کند

#ifdef __cplusplus
}
#endif
