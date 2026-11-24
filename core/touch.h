#pragma once
// تبدیل مختصات خام لمس به مختصات پیکسل (کالیبراسیون سه‌نقطه‌ای). هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// متغیر دیباگ ضرایب کالیبراسیون لمس (بند ۸.۱۵)
extern volatile float mc_dbg_375_touch_coef[6];

#ifdef __cplusplus
}

namespace mc {

// با سه نقطه (مختصات خام لمس و مختصات پیکسل همان نقطه‌ها) ضرایب تبدیل را حساب می‌کند.
// false = نقطه‌ها روی یک خط یا خیلی نزدیک‌اند و محاسبه ممکن نیست.
bool mc_touch_calibrate_3pt(const uint16_t raw_x[3], const uint16_t raw_y[3],
                            const uint16_t scr_x[3], const uint16_t scr_y[3]);
// آیا کالیبراسیون انجام شده است؟
bool mc_touch_is_calibrated();
// مختصات خام را به پیکسل تبدیل می‌کند (در محدوده صفحه بریده می‌شود). false = هنوز کالیبره نشده.
bool mc_touch_apply(uint16_t raw_x, uint16_t raw_y, uint16_t* scr_x, uint16_t* scr_y);

// دریافت ضرایب کالیبراسیون لمس (۶ ضریب float). false = کالیبره نیست (بند ۸.۱۵)
bool mc_touch_get_coef(float out[6]);
// تنظیم ضرایب کالیبراسیون لمس (۶ ضریب float). true = موفق، false = نامعقول (بند ۸.۱۵)
bool mc_touch_set_coef(const float in[6]);

}  // namespace mc
#endif

