#pragma once
// تبدیل میانگین ADC به دمای سلسیوس (بند ۴.۲). بالای adc_sampler.h و hal_thermistor.h کار می‌کند.
// دما به صورت «درجه ×۱۰» (عدد صحیح) نگه داشته می‌شود؛ یعنی 250 = 25.0 درجه.
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "hal_adc.h"
#include "hal_thermistor.h"

namespace mc {

enum class TempStatus : uint8_t {
    NotReady = 0,   // هنوز میانگین معتبر نداریم
    Ok       = 1,   // دما درست محاسبه شد
    Open     = 2,   // سیم سنسور قطع است (ADC نزدیک سقف)
    Short    = 3    // اتصال کوتاه یا دمای غیرممکن (ADC نزدیک صفر)
};

constexpr uint32_t kTempPeriodMs = 100;     // دوره تبدیل
constexpr uint16_t kAdcFullX16   = 65520;   // 4095 × 16
constexpr uint16_t kAdcOpenX16   = 65280;   // میانگین ×۱۶ از این بیشتر یا برابر باشد = قطع (خام ≥ 4080؛ حالت قطع واقعی روی برد ۶۵۴۴۰ تا ۶۵۴۸۶ است)
constexpr uint16_t kAdcShortX16  = 80;      // میانگین ×۱۶ از این کمتر یا برابر باشد = اتصال کوتاه (خام ≤ 5)
constexpr int32_t  kTempMinX10   = -1000;   // کف دما ( -100 درجه )
constexpr int32_t  kTempMaxX10   = 5000;    // سقف دما ( 500 درجه )

// تبدیل خالص بدون سخت‌افزار (برای تست راحت).
// avg_x16: میانگین ×۱۶ ADC. خروجی: دما×۱۰ و مقاومت ترمیستور به اهم (فقط وقتی نتیجه Ok است).
TempStatus mc_temp_convert(const ThermistorParams& p, uint16_t avg_x16, int32_t* temp_x10, uint32_t* ohm);

// وظیفه زمان‌بند: هر ۱۰۰ میلی‌ثانیه دو کانال را تبدیل و متغیرهای دیباگ را به‌روز می‌کند.
void mc_temp_poll();

// آخرین وضعیت یک کانال.
TempStatus mc_temp_status(AdcChannel ch);
// آخرین دمای ×۱۰ یک کانال. فقط وقتی وضعیت Ok است true می‌دهد و *temp_x10 را می‌نویسد.
bool mc_temp_get_x10(AdcChannel ch, int32_t* temp_x10);

}  // namespace mc
