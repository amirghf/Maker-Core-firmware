#pragma once
// صفحه اصلی رابط کاربری (بند ۸.۲).
// فقط از هدرهای core و hal استفاده می‌کند. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "../config/mc_config.h"

namespace mc {

constexpr uint32_t kUiPollMs = MC_UI_POLL_MS;

// راه‌اندازی ماژول صفحه اصلی UI.
void mc_ui_init();

// وظیفه زمان‌بند صفحه اصلی UI (هر kUiPollMs میلی‌ثانیه).
// در هر دور فقط یک مرحله از قاب ثابت یا یک فیلد متغیر را نقاشی می‌کند.
void mc_ui_poll();

// هسته بعد از هر بار نمایش اسپلش این تابع را صدا می‌زند تا نتیجه اسپلش به UI اعلام شود.
void mc_ui_notify_splash(uint32_t result);

}  // namespace mc
