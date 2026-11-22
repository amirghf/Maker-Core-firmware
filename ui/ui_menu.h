#pragma once
// رابط لیست منو با انکودر (بند ۸.۷).
// فقط از هدرهای core و hal استفاده می‌کند. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

namespace mc {

enum class MenuResult : uint8_t {
    Stay = 0,
    Back = 1
};

// آمادگی منو برای نقاشی از اول (انتخاب = ۰، اولین ردیف = ۰، شروع نقاشی مرحله‌ای).
void mc_ui_menu_open();

// وظیفه زمان‌بند منو: در هر دور یک مرحله کوچک نقاشی انجام داده یا ورودی انکودر را می‌خواند.
MenuResult mc_ui_menu_poll();

}  // namespace mc
