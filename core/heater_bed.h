#pragma once
// کنترل هیتر بستر (بند ۴.۵). روش bang-bang با هیسترزیس. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

namespace mc {

// سقف موقت دمای هدف بستر ×۱۰ (1200 = 120 درجه). در بند ۴.۶ با حداکثر دمای واقعی جایگزین می‌شود.
constexpr int32_t kHeaterBedTargetCapX10 = 1200;
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

}  // namespace mc
