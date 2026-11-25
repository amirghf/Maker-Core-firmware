#pragma once
// رابط هشدارها و پیام‌های سیستم (بند ۱۰.۱۳).
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "../config/mc_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// متغیرهای دیباگ (بند ۱۰.۱۳)
extern volatile uint32_t mc_dbg_481_alert_force;      // نوشتنی: 0 = بی‌اثر. 1 = Info، 2 = Warning، 3 = Error نمونه
extern volatile uint32_t mc_dbg_482_alert_raise_cnt;  // خواندنی: تعداد دفعاتی که mc_alert_raise صدا زده شد
extern volatile uint32_t mc_dbg_483_alert_last_level; // خواندنی: سطح آخرین هشدار: 0 = هیچ، 1 = Info، 2 = Warning، 3 = Error
extern volatile uint32_t mc_dbg_484_alert_kill_screen;// خواندنی: 1 = صفحه kill نشان داده شد، 2 = با OK ساکت شد

#ifdef __cplusplus
}

namespace mc {

enum class AlertLevel : uint8_t {
    Info    = 1,
    Warning = 2,
    Error   = 3
};

// راه‌اندازی ماژول هشدار
void mc_alert_init();

// وظیفه زمان‌بند (پایش متغیر تست دیباگ)
void mc_alert_poll();

// ثبت و نمایش هشدار سیستم (بازر + پیام روی نمایشگر + اعلان سریال)
void mc_alert_raise(AlertLevel level, const char* text, const char* detail);

}  // namespace mc
#endif
