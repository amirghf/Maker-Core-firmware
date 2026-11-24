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

// متغیرهای دیباگ عملکردهای صفحه حرکت (بند ۸.۱۶)
extern volatile uint32_t mc_dbg_395_mv_act_cnt;         // خواندنی: تعداد دفعاتی که دکمه‌ها واقعاً کاری انجام دادند
extern volatile uint32_t mc_dbg_396_mv_act_last;        // خواندنی: اندیس آخرین دکمه موفق (0 تا 11؛ 255 = هنوز نشده)
extern volatile uint32_t mc_dbg_397_mv_act_refused;     // خواندنی: تعداد دفعاتی که دکمه رد شد
extern volatile uint32_t mc_dbg_398_mv_act_en;          // نوشتنی: 0 = دکمه‌ها هیچ کاری نمی‌کنند (پیش‌فرض 1)
extern volatile uint32_t mc_dbg_399_mv_refuse_reason;   // خواندنی: علت آخرین رد شدن
extern volatile int32_t  mc_dbg_400_mv_jog_last_um;     // خواندنی: آخرین حرکت دکمه‌های ± محورها، علامت‌دار، به میکرومتر

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
