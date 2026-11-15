#pragma once
// قفل کوتاه وقفه‌ها، مستقل از برد (بند ۵.۷). هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

namespace mc {

// همه وقفه‌ها را خاموش می‌کند و وضعیت قبلی را برمی‌گرداند. فقط برای کارهای خیلی کوتاه (چند میکروثانیه).
uint32_t mc_irq_save();
// وضعیت قبلی وقفه‌ها را که mc_irq_save برگردانده بود، برمی‌گرداند. تو در تو بودن امن است.
void mc_irq_restore(uint32_t state);

}  // namespace mc
