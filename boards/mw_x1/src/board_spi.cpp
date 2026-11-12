// پیاده‌سازی SPI برای برد MW-X1 (SPI2: فلش W25Q64 و وای‌فای روی یک باس).
// فقط پوشه boards مجاز به include کردن هدرهای ST است.
#include "hal_spi.h"
#include "hal_gpio.h"
#include "pins.h"   // stm32f4xx_hal.h را هم می‌آورد
#include "spi.h"    // hspi2 که CubeMX ساخته

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile int     mc_spi_selftest_ok = 0;  // 1 = سلف‌تست پاس شد
volatile uint8_t mc_dbg_flash_mfr   = 0;  // سازنده فلش (W25Q64: باید 0xEF باشد)
volatile uint8_t mc_dbg_flash_type  = 0;  // نوع فلش (W25Q64: 0x40)
volatile uint8_t mc_dbg_flash_cap   = 0;  // ظرفیت فلش (W25Q64: 0x17)
}

namespace mc {

bool mc_spi_transfer(SpiBus bus, const uint8_t* tx, uint8_t* rx, uint16_t len, uint32_t timeout_ms) {
    if (bus != SpiBus::Spi2Flash) { return false; }
    if ((tx == nullptr) || (rx == nullptr) || (len == 0U) || (len > 64U)) { return false; }
    return HAL_SPI_TransmitReceive(&hspi2, const_cast<uint8_t*>(tx), rx, len, timeout_ms) == HAL_OK;
}

bool mc_spi_selftest() {
    // وای‌فای و فلش هر دو را قبل از کار «انتخاب‌نشده» (CS = HIGH) می‌کنیم.
    mc_gpio_set_mode(PinId::WifiCs, PinMode::Output);
    mc_gpio_write(PinId::WifiCs, true);
    mc_gpio_set_mode(PinId::FlashCs, PinMode::Output);
    mc_gpio_write(PinId::FlashCs, true);

    // دستور 0x9F = خواندن شناسه JEDEC. سه بایت جواب: سازنده، نوع، ظرفیت.
    const uint8_t tx[4] = { 0x9F, 0xFF, 0xFF, 0xFF };
    uint8_t rx[4] = { 0, 0, 0, 0 };

    mc_gpio_write(PinId::FlashCs, false);
    const bool ok = mc_spi_transfer(SpiBus::Spi2Flash, tx, rx, 4U, 20U);
    mc_gpio_write(PinId::FlashCs, true);

    mc_dbg_flash_mfr  = rx[1];
    mc_dbg_flash_type = rx[2];
    mc_dbg_flash_cap  = rx[3];

    const bool pass = ok && (rx[1] == 0xEF);
    mc_spi_selftest_ok = pass ? 1 : 0;
    return pass;
}

}  // namespace mc

extern "C" int mc_spi_selftest_c(void) {
    return mc::mc_spi_selftest() ? 1 : 0;
}
