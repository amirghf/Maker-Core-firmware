#pragma once
// خواندن اندستاپ‌ها (بند ۶.۱): اندستاپ حداقل X و Y و Z.
// هر سه NC هستند: آزاد = LOW ، فشرده یا قطع‌سیم = HIGH.
// این ماژول فقط می‌خواند و debounce می‌کند و می‌شمارد؛ به حرکت و G-code وصل نیست.
// بند ۶.۲ و ۶.۳ از mc_endstop_triggered استفاده می‌کنند.
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

namespace mc {

constexpr uint8_t  kEndstopAxes       = 3U;   // X=0 ، Y=1 ، Z=2 (مثل kAxisX و kAxisY و kAxisZ)
constexpr uint32_t kEndstopPollMs     = 1U;   // دوره وظیفه زمان‌بند (میلی‌ثانیه)
constexpr uint32_t kEndstopDebounceMs = 5U;   // حداقل مدت پایداری سطح برای پذیرفتن تغییر (میلی‌ثانیه)

// یک‌بار هنگام راه‌اندازی.
void mc_endstop_init();
// وظیفه زمان‌بند (هر kEndstopPollMs): پین‌ها را می‌خواند و debounce و متغیرهای debug را به‌روز می‌کند.
void mc_endstop_poll();

// سطح خام پین همان لحظه (HIGH = true). شماره محور بد (غیر از ۰ تا ۲) ← false.
bool mc_endstop_raw(uint8_t axis);
// وضعیت پایدار بعد از debounce: true = اندستاپ زده شده (یا سیمش قطع است). شماره محور بد ← false.
bool mc_endstop_triggered(uint8_t axis);
// تعداد دفعاتی که اندستاپ این محور از آزاد به فشرده رفته (بعد از debounce). شماره محور بد ← ۰.
uint32_t mc_endstop_hits(uint8_t axis);

}  // namespace mc
