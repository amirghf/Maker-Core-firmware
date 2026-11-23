#pragma once
// رابط لیست منو با انکودر (بند ۸.۷).
// فقط از هدرهای core و hal استفاده می‌کند. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// متغیرهای دیباگ منو (بند ۸.۷)
extern volatile uint32_t mc_dbg_324_menu_sel_shown;   // خواندنی: اندیس آیتم هایلایت‌شده منو (۰ تا ۷)
extern volatile uint32_t mc_dbg_325_menu_first_shown; // خواندنی: اندیس اولین ردیف قابل مشاهده در منو (۰ تا ۳)
extern volatile uint32_t mc_dbg_326_menu_enter_cnt;   // خواندنی: تعداد اینترهای منو روی آیتم‌های غیر BACK
extern volatile uint32_t mc_dbg_327_menu_enter_idx;   // خواندنی: اندیس آیتم آخرین اینتر (۲۵۵ = هنوز نشده)
extern volatile uint32_t mc_dbg_328_menu_force;       // نوشتنی: ۰ = بی‌اثر؛ ۱ = ورود به منو؛ ۲ = برگشت به صفحه اصلی؛ ۳ تا ۱۰ = انتخاب آیتم

#ifdef __cplusplus
}

namespace mc {

enum class MenuResult : uint8_t {
    Stay = 0,
    Back = 1,
    OpenMove = 2,
    OpenExtruder = 3,
    OpenBed = 4,
    OpenSensors = 5,
    OpenSd = 6,
    OpenOtg = 7,
    OpenAbout = 8
};

// آمادگی منو برای نقاشی از اول (انتخاب = ۰، اولین ردیف = ۰، شروع نقاشی مرحله‌ای).
void mc_ui_menu_open();

// آمادگی منو با انتخاب آیتم مشخص (بند ۸.۹).
void mc_ui_menu_open_at(uint8_t item_idx);

// وظیفه زمان‌بند منو: در هر دور یک مرحله کوچک نقاشی انجام داده یا ورودی انکودر را می‌خواند.
MenuResult mc_ui_menu_poll();

}  // namespace mc
#endif
