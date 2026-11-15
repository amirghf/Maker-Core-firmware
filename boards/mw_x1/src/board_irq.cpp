// پیاده‌سازی hal_irq.h برای برد MW-X1 (بند ۵.۷). فقط پوشه boards مجاز به include کردن هدرهای ST است.
#include "hal_irq.h"
#include "stm32f4xx_hal.h"

namespace mc {

uint32_t mc_irq_save() {
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

void mc_irq_restore(uint32_t state) {
    if (state == 0U) {
        __enable_irq();
    }
}

}  // namespace mc
