// حالت امن برد MW-X1: همه خروجی‌ها هنگام روشن‌شدن خاموش.
// فقط پوشه boards مجاز به include کردن هدرهای ST است.
#include "hal_board.h"
#include "pins.h"

namespace mc {
namespace {

struct SafeOutput {
    GPIO_TypeDef* port;
    uint16_t      pin;
    GPIO_PinState level;
    uint32_t      speed;
};

// سطح امن هر خروجی هنگام روشن‌شدن
const SafeOutput kSafeOutputs[] = {
    // Enable درایورها فعال LOW است، پس HIGH یعنی غیرفعال
    { MC_X_EN_PORT,  MC_X_EN_PIN,  GPIO_PIN_SET,   GPIO_SPEED_FREQ_LOW },
    { MC_Y_EN_PORT,  MC_Y_EN_PIN,  GPIO_PIN_SET,   GPIO_SPEED_FREQ_LOW },
    { MC_Z_EN_PORT,  MC_Z_EN_PIN,  GPIO_PIN_SET,   GPIO_SPEED_FREQ_LOW },
    { MC_EX_EN_PORT, MC_EX_EN_PIN, GPIO_PIN_SET,   GPIO_SPEED_FREQ_LOW },
    // STEP و DIR
    { MC_X_STEP_PORT,  MC_X_STEP_PIN,  GPIO_PIN_RESET, GPIO_SPEED_FREQ_VERY_HIGH },
    { MC_Y_STEP_PORT,  MC_Y_STEP_PIN,  GPIO_PIN_RESET, GPIO_SPEED_FREQ_VERY_HIGH },
    { MC_Z_STEP_PORT,  MC_Z_STEP_PIN,  GPIO_PIN_RESET, GPIO_SPEED_FREQ_VERY_HIGH },
    { MC_EX_STEP_PORT, MC_EX_STEP_PIN, GPIO_PIN_RESET, GPIO_SPEED_FREQ_VERY_HIGH },
    { MC_X_DIR_PORT,   MC_X_DIR_PIN,   GPIO_PIN_RESET, GPIO_SPEED_FREQ_LOW },
    { MC_Y_DIR_PORT,   MC_Y_DIR_PIN,   GPIO_PIN_RESET, GPIO_SPEED_FREQ_LOW },
    { MC_Z_DIR_PORT,   MC_Z_DIR_PIN,   GPIO_PIN_RESET, GPIO_SPEED_FREQ_LOW },
    { MC_EX_DIR_PORT,  MC_EX_DIR_PIN,  GPIO_PIN_RESET, GPIO_SPEED_FREQ_LOW },
    // هیتر و فن (فعال HIGH، پس LOW = خاموش)
    { MC_HEATER_EX_PORT,  MC_HEATER_EX_PIN,  GPIO_PIN_RESET, GPIO_SPEED_FREQ_LOW },
    { MC_HEATER_BED_PORT, MC_HEATER_BED_PIN, GPIO_PIN_RESET, GPIO_SPEED_FREQ_LOW },
    { MC_FAN_FILAMENT_PORT, MC_FAN_FILAMENT_PIN, GPIO_PIN_RESET, GPIO_SPEED_FREQ_LOW },
    // بیزر، بک‌لایت و سروو BLTouch
    { MC_BUZZER_PORT,        MC_BUZZER_PIN,        GPIO_PIN_RESET, GPIO_SPEED_FREQ_LOW },
    { MC_LCD_BLK_PORT,       MC_LCD_BLK_PIN,       GPIO_PIN_RESET, GPIO_SPEED_FREQ_LOW },
    { MC_BLTOUCH_SERVO_PORT, MC_BLTOUCH_SERVO_PIN, GPIO_PIN_RESET, GPIO_SPEED_FREQ_LOW },
    // CSها در حالت بیکاری HIGH (فعال LOW)
    { MC_LCD_CS_PORT,   MC_LCD_CS_PIN,   GPIO_PIN_SET, GPIO_SPEED_FREQ_LOW },
    { MC_TOUCH_CS_PORT, MC_TOUCH_CS_PIN, GPIO_PIN_SET, GPIO_SPEED_FREQ_LOW },
    { MC_SD_CS_PORT,    MC_SD_CS_PIN,    GPIO_PIN_SET, GPIO_SPEED_FREQ_LOW },
    { MC_FL_CS_PORT,    MC_FL_CS_PIN,    GPIO_PIN_SET, GPIO_SPEED_FREQ_LOW },
    { MC_CS_WIFI_PORT,  MC_CS_WIFI_PIN,  GPIO_PIN_SET, GPIO_SPEED_FREQ_LOW },
};

constexpr unsigned kSafeOutputCount =
    sizeof(kSafeOutputs) / sizeof(kSafeOutputs[0]);

// کلاک همه پورت‌های لازم را روشن می‌کند
void mc_enable_gpio_clocks() {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
}

}  // namespace

void mc_board_safe_state() {
    mc_enable_gpio_clocks();
    for (unsigned i = 0; i < kSafeOutputCount; ++i) {
        const SafeOutput& o = kSafeOutputs[i];
        // اول سطح امن را می‌نویسیم، بعد حالت پین را خروجی می‌کنیم
        // (اگر پین قبلاً AF تایمر بوده، بدون پرش به خروجی LOW می‌رود)
        HAL_GPIO_WritePin(o.port, o.pin, o.level);
        GPIO_InitTypeDef init = {};
        init.Pin   = o.pin;
        init.Mode  = GPIO_MODE_OUTPUT_PP;
        init.Pull  = GPIO_NOPULL;
        init.Speed = o.speed;
        HAL_GPIO_Init(o.port, &init);
    }
}

bool mc_board_safe_state_check() {
    for (unsigned i = 0; i < kSafeOutputCount; ++i) {
        const SafeOutput& o = kSafeOutputs[i];
        const unsigned index = static_cast<unsigned>(__builtin_ctz(o.pin));
        // حالت پین باید خروجی (01) باشد
        const uint32_t mode = (o.port->MODER >> (index * 2U)) & 0x3U;
        if (mode != 0x1U) {
            return false;
        }
        // سطح پین باید برابر سطح امن باشد
        if (HAL_GPIO_ReadPin(o.port, o.pin) != o.level) {
            return false;
        }
    }
    return true;
}

}  // namespace mc

// پل‌های C برای main.c (استثنای مجاز از قانون namespace)
extern "C" void mc_board_safe_state_c(void) {
    mc::mc_board_safe_state();
}

extern "C" int mc_board_safe_state_check_c(void) {
    return mc::mc_board_safe_state_check() ? 1 : 0;
}
