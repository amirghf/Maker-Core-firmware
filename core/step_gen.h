#pragma once
// تولیدکننده پالس STEP با وقفه تایمر برای یک محور (بند ۵.۲). هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "hal_stepper.h"

namespace mc {

// یک اجرا را شروع می‌کند: جهت را می‌گذارد و steps پالس با نرخ rate_hz از وقفه تایمر می‌دهد.
// false (و بدون هیچ پالسی) اگر: برد kill شده، محور نامعتبر، درایور آن محور غیرفعال،
// steps صفر، نرخ بیرون از بازه hal، یا یک اجرای دیگر در جریان باشد.
bool mc_stepgen_start(StepAxis axis, bool forward, uint32_t steps, uint32_t rate_hz);
// اجرای جاری را متوقف می‌کند (بی‌اثر اگر در جریان نباشد).
void mc_stepgen_stop();
// نرخ را حین اجرا عوض می‌کند. false اگر اجرا در جریان نیست یا نرخ نامعتبر است.
bool mc_stepgen_set_rate(uint32_t rate_hz);
// آیا اجرا در جریان است؟
bool mc_stepgen_busy();
// تعداد گام باقی‌مانده.
uint32_t mc_stepgen_remaining();
// هوک وقفه را ثبت می‌کند و حالت را صفر می‌کند. یک‌بار هنگام راه‌اندازی.
void mc_stepgen_init();
// وظیفه زمان‌بند (هر ۱۰ میلی‌ثانیه): تست با متغیرهای debug و به‌روزرسانی آن‌ها.
void mc_stepgen_poll();

}  // namespace mc
