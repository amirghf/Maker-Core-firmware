// پیاده‌سازی لمس برای برد MW-X1: تراشه ADS7843E روی SPI1 که با نمایشگر مشترک است.
// فقط پوشه boards مجاز به include کردن هدرهای ST است.
#include "hal_touch.h"
#include "hal_gpio.h"
#include "pins.h"   // stm32f4xx_hal.h را هم می‌آورد
#include "spi.h"    // hspi1 که CubeMX ساخته

// متغیر دیباگ؛ در CubeIDE با Live Expressions دیده می‌شود.
extern "C" {
volatile uint32_t mc_dbg_touch_spi_fail = 0;   // تعداد ارسال‌های SPI لمس که خطا دادند
}

namespace mc {
namespace {

constexpr uint32_t kTouchPrescaler = SPI_BAUDRATEPRESCALER_64;  // حدود ۱.۳ مگاهرتز (حداکثر تراشه ۲ مگاهرتز)
constexpr uint32_t kSpiTimeoutMs   = 20U;
constexpr uint8_t  kCmdX           = 0xD0U;   // دستور خواندن محور X (۱۲ بیت)
constexpr uint8_t  kCmdY           = 0x90U;   // دستور خواندن محور Y (۱۲ بیت)
constexpr uint8_t  kSamples        = 7U;      // تعداد نمونه برای هر محور

bool g_inited = false;

// سرعت SPI1 را عوض می‌کند (فقط بیت‌های BR در CR1).
void set_spi_speed(uint32_t prescaler) {
    __HAL_SPI_DISABLE(&hspi1);
    MODIFY_REG(hspi1.Instance->CR1, SPI_CR1_BR, prescaler);
    __HAL_SPI_ENABLE(&hspi1);
}

// یک تبدیل ۱۲ بیتی برای یک محور.
bool read_channel(uint8_t cmd, uint16_t* out) {
    const uint8_t tx[3] = { cmd, 0U, 0U };
    uint8_t rx[3] = { 0U, 0U, 0U };
    if (HAL_SPI_TransmitReceive(&hspi1, const_cast<uint8_t*>(tx), rx, 3U, kSpiTimeoutMs) != HAL_OK) {
        mc_dbg_touch_spi_fail = mc_dbg_touch_spi_fail + 1U;
        return false;
    }
    const uint16_t raw = static_cast<uint16_t>((static_cast<uint16_t>(rx[1]) << 8) | rx[2]);
    *out = static_cast<uint16_t>((raw >> 3) & 0x0FFFU);
    return true;
}

// چند نمونه می‌گیرد، مرتب می‌کند و میانگین سه مقدار وسط را برمی‌گرداند (حذف نویز).
bool read_axis(uint8_t cmd, uint16_t* out) {
    uint16_t v[kSamples];
    for (uint8_t i = 0U; i < kSamples; i = static_cast<uint8_t>(i + 1U)) {
        if (!read_channel(cmd, &v[i])) { return false; }
    }
    for (uint8_t i = 1U; i < kSamples; i = static_cast<uint8_t>(i + 1U)) {
        const uint16_t key = v[i];
        int j = static_cast<int>(i) - 1;
        while ((j >= 0) && (v[j] > key)) {
            v[j + 1] = v[j];
            j = j - 1;
        }
        v[j + 1] = key;
    }
    const uint32_t sum = static_cast<uint32_t>(v[2]) + v[3] + v[4];
    *out = static_cast<uint16_t>(sum / 3U);
    return true;
}

}  // namespace

bool mc_touch_init() {
    mc_gpio_set_mode(PinId::TouchCs, PinMode::Output);
    mc_gpio_write(PinId::TouchCs, true);                 // لمس انتخاب نشود
    mc_gpio_set_mode(PinId::TouchIrq, PinMode::InputPullUp);
    g_inited = true;
    return true;
}

bool mc_touch_pressed() {
    return !mc_gpio_read(PinId::TouchIrq);   // PEN فعال LOW است
}

bool mc_touch_read_raw(uint16_t* x, uint16_t* y) {
    if ((!g_inited) || (x == nullptr) || (y == nullptr)) { return false; }
    if (!mc_touch_pressed()) { return false; }

    // سرعت فعلی SPI (سرعت نمایشگر) را نگه می‌داریم و بعد از خواندن برمی‌گردانیم.
    const uint32_t old_speed = hspi1.Instance->CR1 & SPI_CR1_BR;
    set_spi_speed(kTouchPrescaler);
    mc_gpio_write(PinId::TouchCs, false);

    uint16_t rx = 0U;
    uint16_t ry = 0U;
    const bool ok = read_axis(kCmdX, &rx) && read_axis(kCmdY, &ry);

    mc_gpio_write(PinId::TouchCs, true);
    set_spi_speed(old_speed);

    if (!ok) { return false; }
    if (!mc_touch_pressed()) { return false; }   // وسط خواندن انگشت برداشته شد
    *x = rx;
    *y = ry;
    return true;
}

}  // namespace mc
