#pragma once
// خروجی‌های استپر در هسته (بند ۵.۱). هیچ هدر ST اینجا include نمی‌شود.
#include "hal_stepper.h"

namespace mc {

// همه درایورها را غیرفعال می‌کند. یک‌بار هنگام راه‌اندازی صدا زده شود.
void mc_stepper_out_init();
// یک درایور را فعال/غیرفعال می‌کند. اگر برد kill شده یا محور نامعتبر باشد false برمی‌گرداند و کاری نمی‌کند.
bool mc_motor_enable(StepAxis axis, bool on);
// همه درایورها را غیرفعال می‌کند.
void mc_motor_disable_all();
// وظیفه زمان‌بند (هر ۱۰ میلی‌ثانیه): تست با متغیرهای debug و به‌روزرسانی آن‌ها.
void mc_stepper_out_poll();

}  // namespace mc
