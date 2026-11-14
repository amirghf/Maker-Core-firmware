// پیاده‌سازی خروجی‌های استپر برای برد MW-X1 (بند ۵.۱). فقط پوشه boards مجاز به include کردن هدرهای ST است.
#include "hal_stepper.h"
#include "hal_gpio.h"
#include "stm32f4xx_hal.h"

namespace mc {
namespace {

struct AxisPins {
    PinId step;
    PinId dir;
    PinId en;
};

// ترتیب X، Y، Z، E پایه جدول PinId در board_gpio.cpp است.
const AxisPins kAxisPins[4] = {
    { PinId::XStep,  PinId::XDir,  PinId::XEn },
    { PinId::YStep,  PinId::YDir,  PinId::YEn },
    { PinId::ZStep,  PinId::ZDir,  PinId::ZEn },
    { PinId::EStep,  PinId::EDir,  PinId::EEn }
};

// سمت درست هر محور در بند ۵.۳ مشخص می‌شود
const bool kDirInvert[4] = { false, false, false, false };

inline bool valid_axis(StepAxis axis) {
    return static_cast<uint8_t>(axis) < static_cast<uint8_t>(StepAxis::Count);
}

void init_dwt_once() {
    static bool inited = false;
    if (!inited) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0U;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
        inited = true;
    }
}

void delay_us(uint32_t us) {
    init_dwt_once();
    const uint32_t start = DWT->CYCCNT;
    const uint32_t ticks = (SystemCoreClock / 1000000U) * us;
    while ((DWT->CYCCNT - start) < ticks) {}
}

}  // namespace

void mc_stepper_enable(StepAxis axis, bool on) {
    if (!valid_axis(axis)) { return; }
    const uint8_t idx = static_cast<uint8_t>(axis);
    // A4988: EN پین روی LOW یعنی درایور فعال
    mc_gpio_write(kAxisPins[idx].en, !on);
}

bool mc_stepper_is_enabled(StepAxis axis) {
    if (!valid_axis(axis)) { return false; }
    const uint8_t idx = static_cast<uint8_t>(axis);
    return !mc_gpio_read(kAxisPins[idx].en);
}

void mc_stepper_set_dir(StepAxis axis, bool forward) {
    if (!valid_axis(axis)) { return; }
    const uint8_t idx = static_cast<uint8_t>(axis);
    const bool phys = forward ^ kDirInvert[idx];
    mc_gpio_write(kAxisPins[idx].dir, phys);
    delay_us(2U);
}

bool mc_stepper_get_dir(StepAxis axis) {
    if (!valid_axis(axis)) { return false; }
    const uint8_t idx = static_cast<uint8_t>(axis);
    const bool phys = mc_gpio_read(kAxisPins[idx].dir);
    return phys ^ kDirInvert[idx];
}

void mc_stepper_pulse(StepAxis axis) {
    if (!valid_axis(axis)) { return; }
    const uint8_t idx = static_cast<uint8_t>(axis);
    mc_gpio_write(kAxisPins[idx].step, true);
    delay_us(2U);
    mc_gpio_write(kAxisPins[idx].step, false);
    delay_us(2U);
}

bool mc_stepper_step_level(StepAxis axis) {
    if (!valid_axis(axis)) { return false; }
    const uint8_t idx = static_cast<uint8_t>(axis);
    return mc_gpio_read(kAxisPins[idx].step);
}

}  // namespace mc
