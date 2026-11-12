// پیاده‌سازی فلش W25Q64 برای برد MW-X1 روی SPI2 (مشترک با وای‌فای).
// فقط پوشه boards مجاز به include کردن هدرهای ST است.
#include "hal_flash.h"
#include "hal_gpio.h"
#include "pins.h"   // stm32f4xx_hal.h را هم می‌آورد
#include "spi.h"    // hspi2 که CubeMX ساخته

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_flash_id       = 0;   // شناسه فلش (W25Q64 باید 0xEF4017 باشد)
volatile uint32_t mc_dbg_flash_spi_fail = 0;   // تعداد ارسال‌های SPI فلش که خطا دادند
}

namespace mc {
namespace {

constexpr uint8_t  kCmdWriteEnable  = 0x06U;
constexpr uint8_t  kCmdReadStatus   = 0x05U;
constexpr uint8_t  kCmdRead         = 0x03U;
constexpr uint8_t  kCmdPageProgram  = 0x02U;
constexpr uint8_t  kCmdSectorErase  = 0x20U;
constexpr uint8_t  kCmdJedecId      = 0x9FU;

constexpr uint32_t kCapacity         = 8UL * 1024UL * 1024UL;   // ۸ مگابایت
constexpr uint32_t kSpiTimeoutMs     = 100U;
constexpr uint32_t kProgramTimeoutMs = 50U;     // نوشتن یک صفحه (حداکثر حدود ۳ میلی‌ثانیه)
constexpr uint32_t kEraseTimeoutMs   = 1000U;   // پاک‌کردن سکتور (حداکثر حدود ۴۰۰ میلی‌ثانیه)

bool g_ready = false;

void select_flash()   { mc_gpio_write(PinId::FlashCs, false); }
void deselect_flash() { mc_gpio_write(PinId::FlashCs, true); }

bool tx_bytes(const uint8_t* data, uint16_t len) {
    if (HAL_SPI_Transmit(&hspi2, const_cast<uint8_t*>(data), len, kSpiTimeoutMs) != HAL_OK) {
        mc_dbg_flash_spi_fail = mc_dbg_flash_spi_fail + 1U;
        return false;
    }
    return true;
}

bool rx_bytes(uint8_t* data, uint16_t len) {
    if (HAL_SPI_Receive(&hspi2, data, len, kSpiTimeoutMs) != HAL_OK) {
        mc_dbg_flash_spi_fail = mc_dbg_flash_spi_fail + 1U;
        return false;
    }
    return true;
}

// دستور و آدرس ۳ بایتی را در b (۴ بایت) می‌گذارد.
void make_cmd(uint8_t* b, uint8_t cmd, uint32_t addr) {
    b[0] = cmd;
    b[1] = static_cast<uint8_t>(addr >> 16);
    b[2] = static_cast<uint8_t>(addr >> 8);
    b[3] = static_cast<uint8_t>(addr);
}

// رجیستر وضعیت ۱ را می‌خواند.
bool read_status(uint8_t* st) {
    const uint8_t cmd = kCmdReadStatus;
    select_flash();
    const bool ok = tx_bytes(&cmd, 1U) && rx_bytes(st, 1U);
    deselect_flash();
    return ok;
}

// منتظر می‌ماند تا فلش بیکار شود (بیت BUSY پاک شود). false = خطا یا تمام شدن زمان.
bool wait_ready(uint32_t timeout_ms) {
    const uint32_t start = HAL_GetTick();
    for (;;) {
        uint8_t st = 0xFFU;
        if (!read_status(&st)) { return false; }
        if ((st & 0x01U) == 0U) { return true; }
        if ((HAL_GetTick() - start) > timeout_ms) { return false; }
    }
}

bool write_enable() {
    const uint8_t cmd = kCmdWriteEnable;
    select_flash();
    const bool ok = tx_bytes(&cmd, 1U);
    deselect_flash();
    return ok;
}

}  // namespace

bool mc_flash_init() {
    g_ready = false;
    // وای‌فای و فلش هر دو «انتخاب‌نشده» (CS = HIGH) باشند.
    mc_gpio_set_mode(PinId::WifiCs, PinMode::Output);
    mc_gpio_write(PinId::WifiCs, true);
    mc_gpio_set_mode(PinId::FlashCs, PinMode::Output);
    mc_gpio_write(PinId::FlashCs, true);

    const uint8_t cmd = kCmdJedecId;
    uint8_t id[3] = { 0U, 0U, 0U };
    select_flash();
    const bool ok = tx_bytes(&cmd, 1U) && rx_bytes(id, 3U);
    deselect_flash();

    mc_dbg_flash_id = (static_cast<uint32_t>(id[0]) << 16) | (static_cast<uint32_t>(id[1]) << 8) | id[2];
    g_ready = ok && (id[0] == 0xEFU) && (id[1] == 0x40U) && (id[2] == 0x17U);
    return g_ready;
}

uint32_t mc_flash_capacity() {
    return g_ready ? kCapacity : 0U;
}

bool mc_flash_read(uint32_t addr, uint8_t* buf, uint32_t len) {
    if ((!g_ready) || (buf == nullptr) || (len == 0U)) { return false; }
    if ((addr >= kCapacity) || (len > (kCapacity - addr))) { return false; }

    uint8_t cmd[4];
    make_cmd(cmd, kCmdRead, addr);
    select_flash();
    bool ok = tx_bytes(cmd, 4U);
    uint32_t done = 0U;
    while (ok && (done < len)) {
        uint32_t n = len - done;
        if (n > 4096U) { n = 4096U; }
        ok = rx_bytes(buf + done, static_cast<uint16_t>(n));
        done += n;
    }
    deselect_flash();
    return ok;
}

bool mc_flash_erase_sector(uint32_t addr) {
    if (!g_ready) { return false; }
    if ((addr >= kCapacity) || ((addr & (kFlashSectorSize - 1U)) != 0U)) { return false; }
    if (!wait_ready(kProgramTimeoutMs)) { return false; }
    if (!write_enable()) { return false; }

    uint8_t cmd[4];
    make_cmd(cmd, kCmdSectorErase, addr);
    select_flash();
    const bool ok = tx_bytes(cmd, 4U);
    deselect_flash();
    if (!ok) { return false; }
    return wait_ready(kEraseTimeoutMs);
}

bool mc_flash_write(uint32_t addr, const uint8_t* data, uint32_t len) {
    if ((!g_ready) || (data == nullptr) || (len == 0U)) { return false; }
    if ((addr >= kCapacity) || (len > (kCapacity - addr))) { return false; }

    uint32_t done = 0U;
    while (done < len) {
        const uint32_t cur = addr + done;
        const uint32_t room = kFlashPageSize - (cur & (kFlashPageSize - 1U));   // جای باقی‌مانده در این صفحه
        uint32_t n = len - done;
        if (n > room) { n = room; }

        if (!wait_ready(kProgramTimeoutMs)) { return false; }
        if (!write_enable()) { return false; }

        uint8_t cmd[4];
        make_cmd(cmd, kCmdPageProgram, cur);
        select_flash();
        const bool ok = tx_bytes(cmd, 4U) && tx_bytes(data + done, static_cast<uint16_t>(n));
        deselect_flash();
        if (!ok) { return false; }
        if (!wait_ready(kProgramTimeoutMs)) { return false; }
        done += n;
    }
    return true;
}

}  // namespace mc
