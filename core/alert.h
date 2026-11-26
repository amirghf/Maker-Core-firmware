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
extern volatile uint32_t mc_dbg_493_alert_repeat_drop_cnt; // خواندنی: تعداد هشدارهای تکراری که محدودکننده دور ریخت (بند ۱۰.۱۳)
extern volatile uint32_t mc_dbg_496_alert_last_id;         // خواندنی: شماره AlertId آخرین هشدار (0 = هیچ) (بند ۱۰.۱۳)
extern volatile uint32_t mc_dbg_497_alert_id_force;        // نوشتنی: 1 تا 16 = همان هشدار را نشان بده. 0 = بی‌اثر (بند ۱۰.۱۳)

#ifdef __cplusplus
}

namespace mc {

enum class AlertLevel : uint8_t {
    Info    = 1,
    Warning = 2,
    Error   = 3
};

// شناسه‌های هشدار سیستم (بند ۱۰.۱۳)
enum class AlertId : uint8_t {
    None                = 0,
    HomingFailed        = 1,
    ProbeFailed         = 2,
    FilChgFailed        = 3,
    FilLoadFailed       = 4,
    RunoutRefused       = 5,
    AutotuneFailed      = 6,
    SettingsSaveFailed  = 7,
    ColdExtrude         = 8,
    SplashFailed        = 9,
    SettingsEepromErr   = 10,
    SettingsInvalid     = 11,
    WatchdogReset       = 12,
    FilChgHotendOff     = 13,
    FilChgParkSkipped   = 14,
    HotendTempNeeded    = 15,
    FilamentNotFound    = 16
};

// راه‌اندازی ماژول هشدار
void mc_alert_init();

// وظیفه زمان‌بند (پایش متغیر تست دیباگ و پرچم‌های ISR)
void mc_alert_poll();

// ثبت و نمایش هشدار سیستم (بازر + پیام روی نمایشگر + اعلان سریال)
void mc_alert_raise(AlertLevel level, const char* text, const char* detail);

// ثبت هشدار با شناسه و محدودکننده تکرار (بند ۱۰.۱۳)
void mc_alert_raise_id(AlertId id, const char* detail = nullptr, uint32_t code = 0U);

// پرچم هشدار اکستروژن سرد برای پایش از زمان‌بند ایمن از ISR (بند ۱۰.۱۳)
void mc_alert_cold_extrude_trigger();

}  // namespace mc
#endif

