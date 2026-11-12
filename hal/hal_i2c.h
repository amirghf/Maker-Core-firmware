#pragma once
// رابط I2C مستقل از برد. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
namespace mc {

// آیا دستگاهی با این آدرس ۷ بیتی روی باس جواب می‌دهد؟
bool mc_i2c_probe(uint8_t addr7, uint32_t timeout_ms);
// نوشتن len بایت به دستگاه. true = موفق.
bool mc_i2c_write(uint8_t addr7, const uint8_t* data, uint16_t len, uint32_t timeout_ms);
// خواندن len بایت از دستگاه. true = موفق.
bool mc_i2c_read(uint8_t addr7, uint8_t* data, uint16_t len, uint32_t timeout_ms);
// تست خودکار I2C. true = سالم.
bool mc_i2c_selftest();

}  // namespace mc

extern "C" {
#endif

// پل برای کد C (main.c)
int mc_i2c_selftest_c(void);   // نتیجه را در mc_i2c_selftest_ok هم می‌گذارد

#ifdef __cplusplus
}
#endif
