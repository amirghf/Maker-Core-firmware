#pragma once
// رابط SPI مستقل از برد. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
namespace mc {

// باس‌های SPI برد. فعلاً فقط باس فلش (SPI2).
enum class SpiBus : uint8_t {
    Spi2Flash = 0,
    Count
};

// ارسال و دریافت هم‌زمان len بایت (حداکثر ۶۴). هر دو بافر باید معتبر باشند.
// کنترل پین CS با فراخواننده است. true = موفق.
bool mc_spi_transfer(SpiBus bus, const uint8_t* tx, uint8_t* rx, uint16_t len, uint32_t timeout_ms);
// تست خودکار SPI با خواندن شناسه فلش (فقط خواندن، چیزی نوشته نمی‌شود). true = سالم.
bool mc_spi_selftest();

}  // namespace mc

extern "C" {
#endif

// پل برای کد C (main.c)
int mc_spi_selftest_c(void);   // نتیجه را در mc_spi_selftest_ok هم می‌گذارد

#ifdef __cplusplus
}
#endif
