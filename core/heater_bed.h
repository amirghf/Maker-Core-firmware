#pragma once
// کنترل هیتر بستر (بند ۴.۵). روش bang-bang با هیسترزیس. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "temp_limits.h"

namespace mc {

// سقف دمای هدف بستر ×۱۰ (1200 = 120 درجه). از حداکثر دما مشتق می‌شود (بند ۴.۶).
constexpr int32_t kHeaterBedTargetCapX10 = kTempBedMaxX10 - kTempBedTargetBufferX10;   // 1200 = 120 درجه (از حداکثر دما مشتق می‌شود، بند ۴.۶)
// بیشترین هیسترزیس مجاز ×۱۰ (100 = 10 درجه)
constexpr int32_t kHeaterBedHystMaxX10 = 100;

enum class HeaterBedState : uint8_t {
    Off       = 0,   // مسلح نیست یا هدف صفر است
    SensorBad = 1,   // سنسور سالم نیست (قطع، اتصال کوتاه، آماده نیست)
    Heating   = 2,   // هیتر روشن است (توان ۲۵۵)
    Holding   = 3    // کنترل فعال است ولی هیتر خاموش است
};

// وظیفه زمان‌بند (هر ۱۰۰ میلی‌ثانیه): توان هیتر بستر را مشخص می‌کند.
// فعلاً هدف و مسلح‌شدن از متغیرهای debug می‌آیند (در بند ۴.۸ تغییر می‌کند).
void mc_heater_bed_poll();

// آیا هیتر بستر «درخواست‌شده» است؟ (مسلح و هدف بیشتر از صفر). برای حفاظت حداقل دما (بند ۴.۶).
bool mc_heater_bed_demanded();

// هدف اعمال‌شده ×۱۰ (بعد از سقف). ۰ یعنی مسلح نیست یا هدف صفر است. برای حفاظت‌های بند ۴.۷.
int32_t mc_heater_bed_target_x10();

// آیا هیتر بستر در «حالت تست» است؟ (mc_dbg_06_hbed_arm برابر ۱). در این حالت M140 و M190 خطا می‌دهند.
bool mc_heater_bed_test_mode();

// هدف G-code را می‌گذارد (×۱۰). منفی ← ۰، بالاتر از سقف ← سقف. مقدار واقعاً ذخیره‌شده را برمی‌گرداند.
int32_t mc_heater_bed_set_target_x10(int32_t t_x10);

}  // namespace mc
