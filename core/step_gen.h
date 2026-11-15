#pragma once
// تولیدکننده پالس STEP با وقفه تایمر برای یک محور (بند ۵.۲). هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "hal_stepper.h"

namespace mc {

// سقف و کف شتاب اجرای دوزنقه‌ای (گام بر ثانیه‌مربع) — بند ۵.۴
constexpr uint32_t kStepAccelMin = 50U;
constexpr uint32_t kStepAccelMax = 20000U;

// یک اجرا را شروع می‌کند: جهت را می‌گذارد و steps پالس با نرخ rate_hz از وقفه تایمر می‌دهد.
// false (و بدون هیچ پالسی) اگر: برد kill شده، محور نامعتبر، درایور آن محور غیرفعال،
// steps صفر، نرخ بیرون از بازه hal، یا یک اجرای دیگر در جریان باشد.
bool mc_stepgen_start(StepAxis axis, bool forward, uint32_t steps, uint32_t rate_hz);
// اجرای دوزنقه‌ای (بند ۵.۴): نرخ از start_hz با شتاب accel_sps2 به cruise_hz می‌رسد و در آخر برمی‌گردد.
// false (و بدون هیچ پالسی) اگر: kill، محور نامعتبر، درایور آن محور غیرفعال، steps صفر، اجرای دیگری در جریان،
// start_hz یا cruise_hz بیرون از بازه hal، start_hz > cruise_hz، یا شتاب بیرون از kStepAccelMin تا kStepAccelMax.
bool mc_stepgen_start_ramp(StepAxis axis, bool forward, uint32_t steps,
                           uint32_t start_hz, uint32_t cruise_hz, uint32_t accel_sps2);
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
