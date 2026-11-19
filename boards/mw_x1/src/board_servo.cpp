// پیاده‌سازی hal_servo.h با TIM1 کانال 1 روی پین PA8 برای برد MW-X1 (بند ۶.۵).
// TIM1 در CubeMX: شمارش ۱ میکروثانیه‌ای (Prescaler = 167) و دوره ۲۰ میلی‌ثانیه (Period = 19999).
// فقط پوشه boards مجاز به include کردن هدرهای ST است.
#include "hal_servo.h"
#include "tim.h"
#include "pins.h"
#include "stm32f4xx_hal.h"

// متغیرهای debug؛ در CubeIDE با Live Expressions دیده می‌شوند. شماره‌ها دائمی‌اند.
extern "C" {
volatile uint32_t mc_dbg_232_servo_ccr      = 0;  // خواندنی: مقدار CCR1 تایمر TIM1 (عرض پالس به میکروثانیه؛ ۰ = بدون پالس)
volatile uint32_t mc_dbg_233_servo_pin_mode = 0;  // خواندنی: حالت پین PA8 (۱ = خروجی GPIO و حالت امن، ۲ = AF تایمر و در حال پالس)
}

namespace mc {
namespace {

volatile bool     g_active   = false;
volatile uint32_t g_pulse_us = 0U;

// پین سروو را به تایمر وصل می‌کند.
void pin_to_timer() {
    GPIO_InitTypeDef init = {};
    init.Pin       = MC_BLTOUCH_SERVO_PIN;
    init.Mode      = GPIO_MODE_AF_PP;
    init.Pull      = GPIO_NOPULL;
    init.Speed     = GPIO_SPEED_FREQ_LOW;
    init.Alternate = GPIO_AF1_TIM1;
    HAL_GPIO_Init(MC_BLTOUCH_SERVO_PORT, &init);
}

// پین سروو را خروجی GPIO با سطح LOW می‌کند (همان حالت ایمن board_safe_state.cpp).
void pin_to_safe_low() {
    HAL_GPIO_WritePin(MC_BLTOUCH_SERVO_PORT, MC_BLTOUCH_SERVO_PIN, GPIO_PIN_RESET);
    GPIO_InitTypeDef init = {};
    init.Pin   = MC_BLTOUCH_SERVO_PIN;
    init.Mode  = GPIO_MODE_OUTPUT_PP;
    init.Pull  = GPIO_NOPULL;
    init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(MC_BLTOUCH_SERVO_PORT, &init);
}

}  // namespace

bool mc_servo_start(uint32_t pulse_us) {
    if ((pulse_us < kServoMinPulseUs) || (pulse_us > kServoMaxPulseUs)) {
        return false;
    }
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse_us);
    if (!g_active) {
        pin_to_timer();
        if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK) {
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0U);
            pin_to_safe_low();
            return false;
        }
        g_active = true;
    }
    g_pulse_us = pulse_us;
    return true;
}

void mc_servo_stop() {
    if (!g_active) {
        return;
    }
    pin_to_safe_low();
    (void)HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0U);
    g_active   = false;
    g_pulse_us = 0U;
}

bool mc_servo_active() {
    return g_active;
}

uint32_t mc_servo_pulse_us() {
    return g_pulse_us;
}

void mc_servo_debug_poll() {
    const unsigned index = static_cast<unsigned>(__builtin_ctz(MC_BLTOUCH_SERVO_PIN));
    mc_dbg_232_servo_ccr      = __HAL_TIM_GET_COMPARE(&htim1, TIM_CHANNEL_1);
    mc_dbg_233_servo_pin_mode = (MC_BLTOUCH_SERVO_PORT->MODER >> (index * 2U)) & 0x3U;
}

}  // namespace mc
