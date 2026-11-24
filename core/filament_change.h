#pragma once
// تعویض فیلامنت (دستور M600 و M108 - بند ۹.۱).
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "gcode_handlers.h"

#ifdef __cplusplus
extern "C" {
#endif

// متغیرهای دیباگ تعویض فیلامنت (بند ۹.۱)
extern volatile uint32_t mc_dbg_434_fchg_state;          // خواندنی: فاز فعلی (0 بیکار، 1 Heating، 2 Retract، 3 Park، 4 Unload، 5 WaitUser، 6 Reheat، 7 Load، 8 Purge، 9 Return)
extern volatile uint32_t mc_dbg_435_fchg_result;         // خواندنی: نتیجه آخرین M600 (0 هنوز نشده، 1 موفق، 2 قطع‌شده با STOP/kill، 3 خطا)
extern volatile uint32_t mc_dbg_436_fchg_reason;         // خواندنی: علت رد/خطا (0 هیچ، 1 killed، 2 arm نیست، 3 هدف دما کم/صفر، 4 پارامتر نامعتبر، 5 حرکت رد شد، 6 گرم‌شدن ناموفق، 7 M600 دیگری فعال است، 8 هیتر در حالت تست)
extern volatile uint32_t mc_dbg_437_fchg_resume_src;     // خواندنی: منبع آخرین «ادامه» پذیرفته‌شده (0 هیچ، 1 دکمه نمایشگر، 2 کلید START، 3 M108)
extern volatile uint32_t mc_dbg_438_fchg_resume_cnt;     // خواندنی: تعداد «ادامه»های پذیرفته‌شده
extern volatile uint32_t mc_dbg_439_fchg_resume_refused; // خواندنی: تعداد «ادامه»هایی که چون سنسور فیلامنت «خالی» می‌گفت رد شدند
extern volatile uint32_t mc_dbg_440_fchg_run_cnt;        // خواندنی: تعداد M600هایی که شروع شدند (از روشن شدن)
extern volatile uint32_t mc_dbg_441_fchg_cooled;         // خواندنی: 1 = هیتر وسط انتظار به خاطر timeout خاموش شده
extern volatile uint32_t mc_dbg_442_fchg_wait_s;         // خواندنی: ثانیه‌های سپری‌شده در انتظار کاربر
extern volatile uint32_t mc_dbg_443_fchg_beep_cnt;       // خواندنی: تعداد بوق‌های یادآوری
extern volatile uint32_t mc_dbg_444_fchg_start_key;      // خواندنی: وضعیت debounce‌شده کلید START (1 = فشرده، 0 = رها)
extern volatile uint32_t mc_dbg_445_fchg_ignore_sensor;  // نوشتنی: 1 = موقع «ادامه» سنسور فیلامنت نادیده گرفته شود (پیش‌فرض 0)
extern volatile uint32_t mc_dbg_446_fchg_tmo_s;          // نوشتنی: مدت نگه‌داشتن هیتر در انتظار به ثانیه (0 = از MC_FILCHG_HEATER_HOLD_S)

#ifdef __cplusplus
}

namespace mc {

// راه‌اندازی ماژول تعویض فیلامنت
void mc_filchg_init();

// وظیفه زمان‌بند (هر ۱۰۰ میلی‌ثانیه): متغیرهای debug را به‌روز می‌کند
void mc_filchg_poll();

// اجرای M600. تا پایان کار Busy برمی‌گرداند و منتظر نمی‌ماند.
GcodeStatus mc_filchg_run(const GcodeCommand& cmd, bool first_call);

// آیا M600 الان فعال است؟ (غیر از Idle)
bool mc_filchg_active();

// فاز جاری (۰ تا ۹)
uint8_t mc_filchg_phase();

// آیا الان در حالت انتظار برای کاربر (فاز ۵) است؟
bool mc_filchg_waiting_user();

// آیا الان درخواست «ادامه» پذیرفته می‌شود؟ (true فقط در WaitUser)
bool mc_filchg_can_resume();

// درخواست «ادامه» (src: 1 = دکمه نمایشگر، 2 = کلید START، 3 = M108)
void mc_filchg_resume_request(uint8_t src);

// لغو M600 (برای quick stop و kill)
void mc_filchg_abort();

}  // namespace mc
#endif
