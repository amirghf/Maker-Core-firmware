// پیاده‌سازی I2C برای برد MW-X1 (I2C1 روی PB6 و PB7، EEPROM در آدرس 0x50).
// فقط پوشه boards مجاز به include کردن هدرهای ST است.
#include "hal_i2c.h"
#include "pins.h"   // stm32f4xx_hal.h را هم می‌آورد
#include "i2c.h"    // hi2c1 که CubeMX ساخته

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile int mc_i2c_selftest_ok    = 0;  // 1 = سلف‌تست پاس شد
volatile int mc_dbg_i2c_eeprom_ack = 0;  // 1 = EEPROM در آدرس 0x50 جواب داد
volatile int mc_dbg_i2c_absent_ack = 0;  // باید 0 باشد (آدرس 0x30 دستگاهی ندارد)
}

namespace mc {
namespace {
constexpr uint8_t kEepromAddr = 0x50;
constexpr uint8_t kAbsentAddr = 0x30;
}  // namespace

bool mc_i2c_probe(uint8_t addr7, uint32_t timeout_ms) {
    return HAL_I2C_IsDeviceReady(&hi2c1, static_cast<uint16_t>(addr7 << 1), 2U, timeout_ms) == HAL_OK;
}

bool mc_i2c_write(uint8_t addr7, const uint8_t* data, uint16_t len, uint32_t timeout_ms) {
    if ((data == nullptr) || (len == 0U)) { return false; }
    return HAL_I2C_Master_Transmit(&hi2c1, static_cast<uint16_t>(addr7 << 1),
                                   const_cast<uint8_t*>(data), len, timeout_ms) == HAL_OK;
}

bool mc_i2c_read(uint8_t addr7, uint8_t* data, uint16_t len, uint32_t timeout_ms) {
    if ((data == nullptr) || (len == 0U)) { return false; }
    return HAL_I2C_Master_Receive(&hi2c1, static_cast<uint16_t>(addr7 << 1),
                                  data, len, timeout_ms) == HAL_OK;
}

bool mc_i2c_selftest() {
    const bool eeprom = mc_i2c_probe(kEepromAddr, 20U);
    const bool absent = mc_i2c_probe(kAbsentAddr, 20U);
    mc_dbg_i2c_eeprom_ack = eeprom ? 1 : 0;
    mc_dbg_i2c_absent_ack = absent ? 1 : 0;
    const bool ok = eeprom && !absent;
    mc_i2c_selftest_ok = ok ? 1 : 0;
    return ok;
}

}  // namespace mc

extern "C" int mc_i2c_selftest_c(void) {
    return mc::mc_i2c_selftest() ? 1 : 0;
}
