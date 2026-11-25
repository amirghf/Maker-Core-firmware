#pragma once
// پایش وضعیت «در حال چاپ» (بند ۹.۴). هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// متغیرهای دیباگ وضعیت چاپ (بند ۹.۴)
extern volatile uint32_t mc_dbg_461_frun_print_act;  // خواندنی: 1 = الان «در حال چاپ» حساب می‌شود
extern volatile uint32_t mc_dbg_462_frun_fake_print; // نوشتنی: 1 = «در حال چاپ» اجباراً true باشد (برای تست)

#ifdef __cplusplus
}

namespace mc {

// راه‌اندازی ماژول وضعیت چاپ
void mc_print_init();

// وظیفه زمان‌بند (هر ۱۰۰ میلی‌ثانیه): متغیرهای debug را به‌روز می‌کند
void mc_print_poll();

// اعلان «همین الان یک حرکت چاپی پذیرفته شد»
void mc_print_note_extrusion();

// آیا الان دستگاه «در حال چاپ» است؟
bool mc_print_active();

}  // namespace mc
#endif
