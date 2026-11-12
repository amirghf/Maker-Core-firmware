#pragma once
// نمونه‌گیری و میانگین‌گیری ADC (بند ۴.۱). بالای hal_adc.h کار می‌کند.
// هر ۱۰ میلی‌ثانیه از هر کانال یک نمونه می‌خواند و میانگین ۱۶ نمونه آخر را نگه می‌دارد.
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "hal_adc.h"

namespace mc {

constexpr uint8_t  kAdcWindow   = 16;   // تعداد نمونه‌های میانگین
constexpr uint8_t  kAdcMaxFail  = 5;    // این‌همه خطای پشت‌سرهم یعنی کانال نامعتبر است
constexpr uint32_t kAdcPeriodMs = 10;   // فاصله دو نمونه‌گیری

// وظیفه زمان‌بند: هر ۱۰ میلی‌ثانیه صدا زده می‌شود و از هر دو کانال یک نمونه می‌خواند.
void mc_adc_sampler_poll();

// آیا میانگین این کانال قابل‌اعتماد است؟ (۱۶ نمونه سالم جمع شده و ۵ خطای پشت‌سرهم نداشته)
bool mc_adc_sampler_valid(AdcChannel ch);
// آیا هر دو کانال معتبرند؟
bool mc_adc_sampler_ready();
// میانگین ×۱۶ (یعنی جمع ۱۶ نمونه، ۰ تا ۶۵۵۲۰). تقسیم بر ۱۶ = میانگین خام ۱۲ بیتی.
// اگر کانال معتبر نباشد false برمی‌گرداند و *avg_x16 را عوض نمی‌کند.
bool mc_adc_avg_x16(AdcChannel ch, uint16_t* avg_x16);
// میلی‌ثانیه از آخرین نمونه سالم. اگر هنوز نمونه‌ای نبوده ۰xFFFFFFFF.
uint32_t mc_adc_sample_age_ms(AdcChannel ch);

}  // namespace mc
