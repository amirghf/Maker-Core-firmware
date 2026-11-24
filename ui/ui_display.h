#pragma once
// صفحه DISPLAY رابط کاربری (بند ۸.۲۱): تنظیم زمان خاموشی نمایشگر و زمان خروج خودکار از منو.
// فقط از هدرهای core و hal استفاده می‌کند. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// متغیرهای دیباگ صفحه DISPLAY (بند ۸.۲۱)
extern volatile uint32_t mc_dbg_424_disp_force;         // نوشتنی: 0 = بی‌اثر؛ 1 = باز کردن صفحه؛ 2 = برگشت به منو
extern volatile uint32_t mc_dbg_425_disp_sel_shown;     // خواندنی: مورد فوکوس‌شده (0 = SCREEN OFF، 1 = MENU EXIT، 2 = BACK)
extern volatile uint32_t mc_dbg_426_disp_scr_off_s;     // خواندنی: مقدار فعلی زمان خاموشی نمایشگر به ثانیه (0 = هیچ‌وقت)
extern volatile uint32_t mc_dbg_427_disp_menu_exit_s;   // خواندنی: مقدار فعلی زمان خروج خودکار از منو به ثانیه (0 = هیچ‌وقت)
extern volatile uint32_t mc_dbg_430_disp_store_state;   // خواندنی: وضعیت ذخیره (0 بیکار، 1 در جریان، 2 موفق، 3 خطا)
extern volatile uint32_t mc_dbg_431_disp_store_result;  // خواندنی: کد نتیجه ذخیره یا بارگذاری (0 هیچ، 1 موفق، 2 خطای EEPROM، 3 رکورد نامعتبر، 4 verify ناموفق)
extern volatile uint32_t mc_dbg_432_disp_boot_src;      // خواندنی: منبع مقدارها هنگام بوت (0 = پیش‌فرض چون رکوردی نبود، 1 = از EEPROM، 2 = خطای خواندن)
extern volatile uint32_t mc_dbg_433_disp_clear_force;   // نوشتنی: 1 = رکورد EEPROM نامعتبر شود (برای تست؛ بعد خودش صفر می‌شود)

#ifdef __cplusplus
}

namespace mc {

enum class DisplayResult : uint8_t {
    Stay = 0,
    Back = 1
};

// شروع نقاشی مرحله‌ای ورود، خواندن مقدارهای فعلی و دور ریختن کلیک‌ها و اینترهای قدیمی.
void mc_ui_display_open();

// وظیفه صفحه DISPLAY: در هر دور یک تکه کوچک نقاشی کرده یا ورودی انکودر و لمس را می‌خواند.
DisplayResult mc_ui_display_poll();

// در هر دور mc_ui_poll صدا زده می‌شود (در هر صفحه‌ای): متغیرهای نمایشی را تازه می‌کند و ذخیره‌های معلق را انجام می‌دهد.
void mc_ui_display_bg_poll();

}  // namespace mc
#endif
