// پیاده‌سازی GPIO برای برد MW-X1. فقط پوشه boards مجاز به include کردن هدرهای ST است.
#include "hal_gpio.h"
#include "pins.h"

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile int mc_gpio_selftest_ok = 0;  // 1 = تست خودکار پاس شد
volatile int mc_dbg_key_start = -1;    // سطح پین START (آزاد = 1، فشرده = 0)
volatile int mc_dbg_key_stop  = -1;    // سطح پین STOP  (آزاد = 1، فشرده = 0)
volatile int mc_dbg_enc_btn   = -1;    // سطح دکمه انکودر (آزاد = 1، فشرده = 0)
}

namespace mc {
namespace {

struct PinDef {
    GPIO_TypeDef* port;
    uint16_t      pin;
};

// ترتیب دقیقاً مثل enum class PinId در hal_gpio.h
const PinDef kPins[] = {
    { MC_X_STEP_PORT, MC_X_STEP_PIN }, { MC_X_DIR_PORT, MC_X_DIR_PIN }, { MC_X_EN_PORT, MC_X_EN_PIN },
    { MC_Y_STEP_PORT, MC_Y_STEP_PIN }, { MC_Y_DIR_PORT, MC_Y_DIR_PIN }, { MC_Y_EN_PORT, MC_Y_EN_PIN },
    { MC_Z_STEP_PORT, MC_Z_STEP_PIN }, { MC_Z_DIR_PORT, MC_Z_DIR_PIN }, { MC_Z_EN_PORT, MC_Z_EN_PIN },
    { MC_EX_STEP_PORT, MC_EX_STEP_PIN }, { MC_EX_DIR_PORT, MC_EX_DIR_PIN }, { MC_EX_EN_PORT, MC_EX_EN_PIN },
    { MC_HEATER_EX_PORT, MC_HEATER_EX_PIN }, { MC_HEATER_BED_PORT, MC_HEATER_BED_PIN },
    { MC_FAN_FILAMENT_PORT, MC_FAN_FILAMENT_PIN },
    { MC_BUZZER_PORT, MC_BUZZER_PIN }, { MC_LCD_BLK_PORT, MC_LCD_BLK_PIN },
    { MC_LCD_CS_PORT, MC_LCD_CS_PIN }, { MC_LCD_RS_PORT, MC_LCD_RS_PIN }, { MC_LCD_RES_PORT, MC_LCD_RES_PIN },
    { MC_TOUCH_CS_PORT, MC_TOUCH_CS_PIN }, { MC_SD_CS_PORT, MC_SD_CS_PIN },
    { MC_FL_CS_PORT, MC_FL_CS_PIN }, { MC_CS_WIFI_PORT, MC_CS_WIFI_PIN },
    { MC_KEY_START_PORT, MC_KEY_START_PIN }, { MC_KEY_STOP_PORT, MC_KEY_STOP_PIN },
    { MC_ENC_BTN_PORT, MC_ENC_BTN_PIN },
    { MC_X_LIMIT_MIN_PORT, MC_X_LIMIT_MIN_PIN }, { MC_Y_LIMIT_MIN_PORT, MC_Y_LIMIT_MIN_PIN },
    { MC_Z_LIMIT_MIN_PORT, MC_Z_LIMIT_MIN_PIN },
    { MC_FILAMENT_SENSOR_PORT, MC_FILAMENT_SENSOR_PIN }, { MC_BLTOUCH_KEY_PORT, MC_BLTOUCH_KEY_PIN },
    { MC_SD_DET_PORT, MC_SD_DET_PIN }, { MC_TOUCH_IRQ_PORT, MC_TOUCH_IRQ_PIN },
};

constexpr unsigned kPinCount = sizeof(kPins) / sizeof(kPins[0]);
static_assert(kPinCount == static_cast<unsigned>(PinId::Count),
              "تعداد kPins باید با PinId::Count برابر باشد");

inline bool valid(PinId p) {
    return static_cast<unsigned>(p) < kPinCount;
}

inline const PinDef& def(PinId p) {
    return kPins[static_cast<unsigned>(p)];
}

// آیا پین الان در حالت خروجی است؟ (MODER برابر 01)
inline bool is_output(const PinDef& d) {
    const unsigned index = static_cast<unsigned>(__builtin_ctz(d.pin));
    return ((d.port->MODER >> (index * 2U)) & 0x3U) == 0x1U;
}

}  // namespace

void mc_gpio_set_mode(PinId pin, PinMode mode) {
    if (!valid(pin)) { return; }
    const PinDef& d = def(pin);
    GPIO_InitTypeDef init = {};
    init.Pin   = d.pin;
    init.Speed = GPIO_SPEED_FREQ_LOW;
    switch (mode) {
        case PinMode::Output:        init.Mode = GPIO_MODE_OUTPUT_PP; init.Pull = GPIO_NOPULL;   break;
        case PinMode::Input:         init.Mode = GPIO_MODE_INPUT;     init.Pull = GPIO_NOPULL;   break;
        case PinMode::InputPullUp:   init.Mode = GPIO_MODE_INPUT;     init.Pull = GPIO_PULLUP;   break;
        case PinMode::InputPullDown: init.Mode = GPIO_MODE_INPUT;     init.Pull = GPIO_PULLDOWN; break;
    }
    // HAL_GPIO_Init مقدار ODR را تغییر نمی‌دهد، پس سطح قبلی حفظ می‌شود.
    HAL_GPIO_Init(d.port, &init);
}

void mc_gpio_write(PinId pin, bool high) {
    if (!valid(pin)) { return; }
    const PinDef& d = def(pin);
    if (!is_output(d)) { return; }  // ایمنی: روی ورودی چیزی نمی‌نویسیم
    HAL_GPIO_WritePin(d.port, d.pin, high ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

bool mc_gpio_read(PinId pin) {
    if (!valid(pin)) { return false; }
    const PinDef& d = def(pin);
    return HAL_GPIO_ReadPin(d.port, d.pin) == GPIO_PIN_SET;
}

void mc_gpio_toggle(PinId pin) {
    if (!valid(pin)) { return; }
    const PinDef& d = def(pin);
    if (!is_output(d)) { return; }
    HAL_GPIO_TogglePin(d.port, d.pin);
}

bool mc_gpio_selftest() {
    // ۱) هر ردیف جدول باید پورت معتبر و دقیقاً یک بیت پین داشته باشد
    for (unsigned i = 0; i < kPinCount; ++i) {
        const PinDef& d = kPins[i];
        if (d.port == nullptr) { return false; }
        if (d.pin == 0U || (d.pin & (d.pin - 1U)) != 0U) { return false; }
    }
    // ۲) هیچ دو پینی نباید یک پورت و پین مشترک داشته باشند
    for (unsigned i = 0; i < kPinCount; ++i) {
        for (unsigned j = i + 1U; j < kPinCount; ++j) {
            if (kPins[i].port == kPins[j].port && kPins[i].pin == kPins[j].pin) { return false; }
        }
    }
    // ۳) نوشتن و خواندن روی یک خروجی بی‌خطر (بک‌لایت LCD؛ آخرش دوباره خاموش)
    mc_gpio_write(PinId::LcdBlk, true);
    if (!mc_gpio_read(PinId::LcdBlk)) { mc_gpio_write(PinId::LcdBlk, false); return false; }
    mc_gpio_write(PinId::LcdBlk, false);
    if (mc_gpio_read(PinId::LcdBlk)) { return false; }
    // ۴) محافظ: نوشتن روی پین ورودی (کلید START) نباید حالت آن را عوض کند
    mc_gpio_write(PinId::KeyStart, false);
    if (is_output(def(PinId::KeyStart))) { return false; }
    // ۵) شماره نامعتبر نباید خطا بدهد
    if (mc_gpio_read(PinId::Count)) { return false; }
    return true;
}

}  // namespace mc

extern "C" int mc_gpio_selftest_c(void) {
    mc_gpio_selftest_ok = mc::mc_gpio_selftest() ? 1 : 0;
    return mc_gpio_selftest_ok;
}

extern "C" void mc_gpio_debug_poll_c(void) {
    mc_dbg_key_start = mc::mc_gpio_read(mc::PinId::KeyStart) ? 1 : 0;
    mc_dbg_key_stop  = mc::mc_gpio_read(mc::PinId::KeyStop)  ? 1 : 0;
    mc_dbg_enc_btn   = mc::mc_gpio_read(mc::PinId::EncBtn)   ? 1 : 0;
}
