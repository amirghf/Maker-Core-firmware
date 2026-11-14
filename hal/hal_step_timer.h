#pragma once
// تایمر تولید پالس STEP مستقل از برد (بند ۵.۲). هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
namespace mc {

// نرخ‌های مجاز تایمر (پالس در ثانیه)
constexpr uint32_t kStepTimerMinHz = 1U;
constexpr uint32_t kStepTimerMaxHz = 20000U;

// نوع تابع وقفه تایمر. داخل وقفه اجرا می‌شود؛ باید خیلی کوتاه باشد.
using StepTimerFn = void (*)();

// تابعی را که در هر وقفه تایمر صدا زده شود ثبت می‌کند. nullptr = حذف.
void mc_steptimer_set_callback(StepTimerFn fn);
// تایمر را با نرخ rate_hz (بین kStepTimerMinHz و kStepTimerMaxHz) شروع می‌کند.
// اولین وقفه یک دوره کامل بعد از شروع می‌آید. نرخ نامعتبر = false و تایمر شروع نمی‌شود.
bool mc_steptimer_start(uint32_t rate_hz);
// نرخ را حین کار عوض می‌کند (در وقفه بعدی اعمال می‌شود). نرخ نامعتبر یا تایمر خاموش = false.
bool mc_steptimer_set_rate(uint32_t rate_hz);
// تایمر را متوقف می‌کند (وقفه دیگر نمی‌آید). اگر خاموش باشد بی‌اثر است.
void mc_steptimer_stop();
// آیا تایمر الان کار می‌کند؟
bool mc_steptimer_running();

}  // namespace mc

extern "C" {
#endif

// پل برای کد C (main.c). فقط از وقفه TIM2 صدا زده شود.
void mc_steptimer_isr_c(void);

#ifdef __cplusplus
}
#endif
