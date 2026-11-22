#pragma once
// صفحه حرکت رابط کاربری (بند ۸.۸).
// فقط از هدرهای core و hal استفاده می‌کند. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// متغیرهای دیباگ صفحه حرکت (بند ۸.۸)
extern volatile uint32_t mc_dbg_329_move_sel_shown;   // خواندنی: اندیس مورد فوکوس‌شده (۰ تا ۱۲)
extern volatile uint32_t mc_dbg_330_move_enter_cnt;   // خواندنی: تعداد اینترها روی مورد غیر BACK و غیر STEP
extern volatile uint32_t mc_dbg_331_move_enter_idx;   // خواندنی: اندیس مورد آخرین اینتر (۲۵۵ = هنوز نشده)
extern volatile uint32_t mc_dbg_332_move_force;       // نوشتنی: ۰ = بی‌اثر؛ ۱ = باز کردن صفحه حرکت؛ ۲ = برگشت به منو؛ ۳ تا ۱۵ = فوکوس روی مورد
extern volatile uint32_t mc_dbg_333_move_step_shown;  // خواندنی: گام نمایشی: ۰ = ۰.۱، ۱ = ۱ (پیش‌فرض)، ۲ = ۱۰

#ifdef __cplusplus
}

namespace mc {

enum class MoveResult : uint8_t {
    Stay = 0,
    Back = 1
};

// شروع نقاشی مرحله‌ای ورود و صفر کردن کلیک‌ها و اینترهای قدیمی.
void mc_ui_move_open();

// وظیفه زمان‌بند صفحه حرکت: در هر دور یک تکه کوچک نقاشی کرده یا ورودی انکودر را می‌خواند.
MoveResult mc_ui_move_poll();

}  // namespace mc
#endif
