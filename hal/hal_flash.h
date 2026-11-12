#pragma once
// رابط فلش خارجی مستقل از برد. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
namespace mc {

constexpr uint32_t kFlashSectorSize = 4096U;   // اندازه کوچک‌ترین بخش قابل پاک‌کردن
constexpr uint32_t kFlashPageSize   = 256U;    // اندازه یک صفحه نوشتن

// فلش را راه‌اندازی می‌کند و شناسه آن را (W25Q64 = EF 40 17) بررسی می‌کند. true = فلش درست پیدا شد.
bool mc_flash_init();
// ظرفیت فلش به بایت. اگر mc_flash_init موفق نبوده ۰ برمی‌گرداند.
uint32_t mc_flash_capacity();
// len بایت از آدرس addr می‌خواند. true = موفق. آدرس بیرون از ظرفیت = false.
bool mc_flash_read(uint32_t addr, uint8_t* buf, uint32_t len);
// یک سکتور ۴ کیلوبایتی را پاک می‌کند (همه بایت‌ها ۰xFF می‌شوند). addr باید مضرب ۴۰۹۶ باشد.
// تا حدود ۰.۴ ثانیه منتظر می‌ماند. true = موفق.
bool mc_flash_erase_sector(uint32_t addr);
// len بایت می‌نویسد (خودش از مرز صفحه‌ها عبور می‌کند). محدوده باید قبلاً پاک شده باشد. true = موفق.
bool mc_flash_write(uint32_t addr, const uint8_t* data, uint32_t len);

}  // namespace mc
#endif
