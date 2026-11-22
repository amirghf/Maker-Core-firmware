#pragma once
// خواندن خودکار تنظیمات هنگام بوت از EEPROM (بند ۷.۴).
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "../config/mc_config.h"

namespace mc {

constexpr uint32_t kCfgBootPollMs = MC_CFG_BOOT_POLL_MS;

// یک‌بار هنگام راه‌اندازی (حالت را Idle و متغیرهای دیباگ را صفر می‌کند)
void mc_cfg_boot_init();

// وظیفه زمان‌بند (هر kCfgBootPollMs). تنظیمات را خودکار از EEPROM می‌خواند و اعمال می‌کند.
void mc_cfg_boot_poll();

// آیا بوت و خواندن تنظیمات هنوز در جریان است؟ (true = هنوز تمام نشده؛ false = تمام شد)
bool mc_cfg_boot_busy();

}  // namespace mc
