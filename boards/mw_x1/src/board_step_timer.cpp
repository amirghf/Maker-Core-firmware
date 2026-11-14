// پیاده‌سازی hal_step_timer.h با TIM2 برای برد MW-X1 (بند ۵.۲).
// فقط پوشه boards مجاز به include کردن هدرهای ST است.
#include "hal_step_timer.h"
#include "tim.h"
#include "stm32f4xx_hal.h"

namespace mc {
namespace {

constexpr uint32_t kTimerClkHz = 1000000U;   // کلاک شمارش ۱ مگاهرتز (Prescaler=83 در CubeMX)

StepTimerFn volatile g_cb      = nullptr;
bool        volatile g_running = false;

}  // namespace

void mc_steptimer_set_callback(StepTimerFn fn) {
    g_cb = fn;
}

bool mc_steptimer_start(uint32_t rate_hz) {
    if ((rate_hz < kStepTimerMinHz) || (rate_hz > kStepTimerMaxHz)) {
        return false;
    }
    if (g_running) {
        mc_steptimer_stop();
    }

    const uint32_t arr = (kTimerClkHz / rate_hz) - 1U;
    __HAL_TIM_SET_AUTORELOAD(&htim2, arr);
    __HAL_TIM_SET_COUNTER(&htim2, 0U);
    htim2.Instance->EGR = TIM_EGR_UG;
    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);

    if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK) {
        return false;
    }
    g_running = true;
    return true;
}

bool mc_steptimer_set_rate(uint32_t rate_hz) {
    if (!g_running || (rate_hz < kStepTimerMinHz) || (rate_hz > kStepTimerMaxHz)) {
        return false;
    }
    const uint32_t arr = (kTimerClkHz / rate_hz) - 1U;
    __HAL_TIM_SET_AUTORELOAD(&htim2, arr);
    return true;
}

void mc_steptimer_stop() {
    HAL_TIM_Base_Stop_IT(&htim2);
    g_running = false;
}

bool mc_steptimer_running() {
    return g_running;
}

}  // namespace mc

extern "C" void mc_steptimer_isr_c(void) {
    mc::StepTimerFn fn = mc::g_cb;
    if (fn != nullptr) {
        fn();
    } else {
        mc::mc_steptimer_stop();
    }
}
