#pragma once
// رابط ADC مستقل از برد. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
namespace mc {

// کانال‌های آنالوگ برد.
enum class AdcChannel : uint8_t {
    TempExtruder = 0,
    TempBed,
    Count
};

// یک نمونه خام ۱۲ بیتی (۰ تا ۴۰۹۵) می‌خواند. true = موفق.
bool mc_adc_read(AdcChannel ch, uint16_t* raw12);
// تست خودکار ADC. true = هر دو کانال نمونه معتبر دادند.
bool mc_adc_selftest();
// هر ۵۰۰ میلی‌ثانیه یک‌بار دو کانال را می‌خواند و متغیرهای دیباگ را به‌روز می‌کند.
void mc_adc_debug_poll();

}  // namespace mc

extern "C" {
#endif

// پل‌ها برای کد C (main.c)
int  mc_adc_selftest_c(void);      // نتیجه را در mc_adc_selftest_ok هم می‌گذارد
void mc_adc_debug_poll_c(void);    // برای حلقه اصلی main.c

#ifdef __cplusplus
}
#endif
