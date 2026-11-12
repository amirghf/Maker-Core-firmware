#pragma once
// پارامترهای ترمیستور هر کانال دما (بند ۴.۲). مستقل از برد؛ مقدارها را پوشه boards می‌دهد.
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "hal_adc.h"

namespace mc {

// مدل بتا: 1/T = 1/T0 + ln(R/R0)/beta  (T به کلوین)
struct ThermistorParams {
    float pullup_ohm;   // مقاومت پول‌آپ به ۳.۳ ولت (اهم)
    float r0_ohm;       // مقاومت ترمیستور در دمای t0_c (اهم)
    float t0_c;         // دمای مرجع (سلسیوس)
    float beta;         // ضریب بتا
};

// پارامترهای یک کانال را می‌دهد. false = کانال نامعتبر.
bool mc_thermistor_params(AdcChannel ch, ThermistorParams* out);

}  // namespace mc
