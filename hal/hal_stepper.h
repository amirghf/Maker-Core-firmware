#pragma once
// رابط خروجی‌های استپر مستقل از برد (بند ۵.۱). هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

namespace mc {

enum class StepAxis : uint8_t {
    X = 0,
    Y = 1,
    Z = 2,
    E = 3,
    Count = 4   // همیشه آخر؛ تعداد محورها
};

// درایور را فعال یا غیرفعال می‌کند. on = true یعنی موتور قفل می‌شود (A4988: پین EN روی LOW).
// شماره نامعتبر نادیده گرفته می‌شود.
void mc_stepper_enable(StepAxis axis, bool on);
// آیا درایور الان فعال است؟ (از روی سطح واقعی پین EN). شماره نامعتبر = false.
bool mc_stepper_is_enabled(StepAxis axis);
// جهت منطقی: forward = true یعنی جهت مثبت. معکوس‌سازی سخت‌افزاری داخل پیاده‌سازی برد است.
void mc_stepper_set_dir(StepAxis axis, bool forward);
// جهت منطقی فعلی (از روی سطح واقعی پین DIR و معکوس‌سازی). شماره نامعتبر = false.
bool mc_stepper_get_dir(StepAxis axis);
// یک پالس STEP می‌دهد: پین HIGH، حداقل ۲ میکروثانیه صبر، پین LOW، حداقل ۲ میکروثانیه صبر.
// اگر درایور غیرفعال باشد هم پالس می‌دهد؛ ممنوعیت آن با لایه بالاتر است.
void mc_stepper_pulse(StepAxis axis);
// سطح واقعی پین STEP (برای تست؛ در حالت بیکار باید false باشد).
bool mc_stepper_step_level(StepAxis axis);

}  // namespace mc
