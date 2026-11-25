#pragma once
// بارگذاری و تخلیه فیلامنت (دستور M701 و M702 - بند ۹.۲).
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "gcode_handlers.h"

#ifdef __cplusplus
extern "C" {
#endif

// متغیرهای دیباگ بارگذاری و تخلیه فیلامنت (بند ۹.۲)
extern volatile uint32_t mc_dbg_451_fld_state;      // خواندنی: 0 بیکار، 1 Heating، 2 Moving، 3 Purging
extern volatile uint32_t mc_dbg_452_fld_result;     // خواندنی: نتیجه آخرین M701/M702 (0 هنوز نشده، 1 موفق، 2 قطع‌شده با STOP/kill، 3 خطا)
extern volatile uint32_t mc_dbg_453_fld_reason;     // خواندنی: علت رد/خطا (0 هیچ، 1 killed، 2 arm نیست، 3 هدف دما کم/صفر، 4 پارامتر نامعتبر، 5 حرکت رد شد، 6 گرم‌شدن ناموفق، 7 فعال بودن دستور دیگر، 8 هیتر در حالت تست، 9 فیلامنت تشخیص داده نشد)
extern volatile uint32_t mc_dbg_454_fld_load_cnt;   // خواندنی: تعداد M701هایی که شروع شدند
extern volatile uint32_t mc_dbg_455_fld_unload_cnt; // خواندنی: تعداد M702هایی که شروع شدند

#ifdef __cplusplus
}

namespace mc {

// راه‌اندازی ماژول بارگذاری و تخلیه فیلامنت (بند ۹.۲)
void mc_fload_init();

// وظیفه زمان‌بند (هر ۱۰۰ میلی‌ثانیه): متغیرهای debug را به‌روز می‌کند
void mc_fload_poll();

// اجرای M701 (is_load = true) یا M702 (is_load = false).
GcodeStatus mc_fload_run(const GcodeCommand& cmd, bool first_call, bool is_load);

// آیا M701/M702 الان فعال است؟ (غیر از Idle)
bool mc_fload_active();

// لغو M701/M702 (برای quick stop و kill)
void mc_fload_abort();

}  // namespace mc
#endif
