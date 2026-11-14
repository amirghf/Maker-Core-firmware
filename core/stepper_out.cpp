// خروجی‌های استپر در هسته (بند ۵.۱). هیچ هدر ST اینجا include نمی‌شود.
#include "stepper_out.h"
#include "hal_time.h"
#include "error.h"

// متغیرهای debug؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_56_st_arm        = 0;  // برای تست: نوشتنی. ۱ = تست استپر مجاز است. ۰ = همه چیز غیرفعال.
volatile uint32_t mc_dbg_57_st_axis       = 0;  // برای تست: نوشتنی. محور تست: ۰ = X، ۱ = Y، ۲ = Z، ۳ = E.
volatile uint32_t mc_dbg_58_st_enable     = 0;  // برای تست: نوشتنی. ۱ = درایور محور انتخابی فعال باشد، ۰ = غیرفعال.
volatile uint32_t mc_dbg_59_st_dir        = 0;  // برای تست: نوشتنی. جهت منطقی: ۰ = منفی، ۱ = مثبت.
volatile uint32_t mc_dbg_60_st_pulses     = 0;  // برای تست: نوشتنی. تعداد پالسی که باید داده شود؛ با هر پالس یکی کم می‌شود تا صفر.
volatile uint32_t mc_dbg_61_st_per_poll   = 4;  // برای تست: نوشتنی. تعداد پالس در هر وظیفه ۱۰ میلی‌ثانیه‌ای. مقدار اولیه ۴. بازه مجاز ۱ تا ۲۰.
volatile uint32_t mc_dbg_62_st_en_pins    = 0;  // برای تست: خواندنی. ماسک بیتی درایورهای فعال از روی سطح واقعی پین EN: بیت۰ X، بیت۱ Y، بیت۲ Z، بیت۳ E.
volatile uint32_t mc_dbg_63_st_dir_pins   = 0;  // برای تست: خواندنی. ماسک بیتی جهت منطقی چهار محور.
volatile uint32_t mc_dbg_64_st_emitted    = 0;  // برای تست: خواندنی. مجموع پالس‌های داده‌شده از روشن‌شدن برد.
volatile uint32_t mc_dbg_65_st_step_high  = 0;  // برای تست: خواندنی. ماسک بیتی پین‌های STEP که الان HIGH هستند (باید همیشه ۰ باشد).
volatile uint32_t mc_dbg_66_st_refused    = 0;  // برای تست: خواندنی. تعداد درخواست‌های ردشده.
volatile uint32_t mc_dbg_67_st_idle_off   = 0;  // برای تست: خواندنی. تعداد دفعاتی که درایور به‌خاطر بیکاری خودکار غیرفعال شد.
}

namespace mc {
namespace {

constexpr uint32_t kPollMs       = 10U;
constexpr uint32_t kPerPollMin   = 1U;
constexpr uint32_t kPerPollMax   = 20U;
constexpr uint32_t kTestIdleMs   = 30000U;   // فقط برای تست؛ بعداً با M84 و تنظیمات جایگزین می‌شود

uint32_t g_last_activity_ms = 0U;
bool     g_prev_arm_was_one = false;

}  // namespace

void mc_stepper_out_init() {
    mc_motor_disable_all();
    mc_dbg_56_st_arm        = 0U;
    mc_dbg_57_st_axis       = 0U;
    mc_dbg_58_st_enable     = 0U;
    mc_dbg_59_st_dir        = 0U;
    mc_dbg_60_st_pulses     = 0U;
    mc_dbg_61_st_per_poll   = 4U;
    mc_dbg_62_st_en_pins    = 0U;
    mc_dbg_63_st_dir_pins   = 0U;
    mc_dbg_64_st_emitted    = 0U;
    mc_dbg_65_st_step_high  = 0U;
    mc_dbg_66_st_refused    = 0U;
    mc_dbg_67_st_idle_off   = 0U;
    g_last_activity_ms      = mc_millis();
    g_prev_arm_was_one      = false;
}

bool mc_motor_enable(StepAxis axis, bool on) {
    if (mc_is_killed()) { return false; }
    const uint8_t idx = static_cast<uint8_t>(axis);
    if (idx >= static_cast<uint8_t>(StepAxis::Count)) { return false; }
    mc_stepper_enable(axis, on);
    if (on) {
        g_last_activity_ms = mc_millis();
    }
    return true;
}

void mc_motor_disable_all() {
    for (uint8_t i = 0U; i < static_cast<uint8_t>(StepAxis::Count); ++i) {
        mc_stepper_enable(static_cast<StepAxis>(i), false);
    }
}

void mc_stepper_out_poll() {
    // ۱) اگر برد kill شده است فوراً برگرد
    if (mc_is_killed()) { return; }

    // ۳) اصلاح mc_dbg_61_st_per_poll بین ۱ و ۲۰
    if (mc_dbg_61_st_per_poll < kPerPollMin) {
        mc_dbg_61_st_per_poll = kPerPollMin;
    } else if (mc_dbg_61_st_per_poll > kPerPollMax) {
        mc_dbg_61_st_per_poll = kPerPollMax;
    }

    // ۴) اگر arm صفر است
    if (mc_dbg_56_st_arm == 0U) {
        if (g_prev_arm_was_one) {
            mc_motor_disable_all();
            mc_dbg_58_st_enable = 0U;
            mc_dbg_60_st_pulses = 0U;
        }
        if (mc_dbg_60_st_pulses > 0U) {
            mc_dbg_60_st_pulses = 0U;
            mc_dbg_66_st_refused = mc_dbg_66_st_refused + 1U;
        }
        g_prev_arm_was_one = false;
        goto update_debug;
    }

    // ۵) اگر شماره محور نامعتبر است (بزرگ‌تر از ۳)
    if (mc_dbg_57_st_axis > 3U) {
        mc_dbg_57_st_axis = 0U;
        mc_dbg_66_st_refused = mc_dbg_66_st_refused + 1U;
        mc_dbg_58_st_enable = 0U;
        mc_dbg_60_st_pulses = 0U;
        g_prev_arm_was_one = true;
        goto update_debug;
    }

    {
        const StepAxis axis = static_cast<StepAxis>(mc_dbg_57_st_axis);

        // ۶) اعمال mc_dbg_58_st_enable روی محور انتخابی
        if (mc_dbg_58_st_enable == 1U) {
            if (!mc_stepper_is_enabled(axis)) {
                mc_stepper_enable(axis, true);
                g_last_activity_ms = mc_millis();
            }
        } else {
            mc_stepper_enable(axis, false);
        }

        // ۷) اگر پالس درخواست شده است
        if (mc_dbg_60_st_pulses > 0U) {
            if (!mc_stepper_is_enabled(axis)) {
                mc_dbg_60_st_pulses = 0U;
                mc_dbg_66_st_refused = mc_dbg_66_st_refused + 1U;
            } else {
                mc_stepper_set_dir(axis, mc_dbg_59_st_dir != 0U);
                const uint32_t n = (mc_dbg_61_st_per_poll < mc_dbg_60_st_pulses)
                                   ? mc_dbg_61_st_per_poll
                                   : mc_dbg_60_st_pulses;
                for (uint32_t i = 0U; i < n; ++i) {
                    mc_stepper_pulse(axis);
                }
                mc_dbg_60_st_pulses -= n;
                mc_dbg_64_st_emitted += n;
                g_last_activity_ms = mc_millis();
            }
        }

        // ۸) بیکاری خودکار
        if ((mc_dbg_56_st_arm == 1U) && mc_stepper_is_enabled(axis) &&
            (mc_dbg_60_st_pulses == 0U) && mc_time_elapsed_ms(g_last_activity_ms, kTestIdleMs)) {
            mc_motor_disable_all();
            mc_dbg_58_st_enable = 0U;
            mc_dbg_67_st_idle_off = mc_dbg_67_st_idle_off + 1U;
        }
    }

    g_prev_arm_was_one = true;

update_debug:
    // ۹) به‌روزرسانی متغیرهای خواندنی دیباگ از روی سطح واقعی پین‌ها
    {
        uint32_t en_pins   = 0U;
        uint32_t dir_pins  = 0U;
        uint32_t step_high = 0U;
        for (uint8_t i = 0U; i < static_cast<uint8_t>(StepAxis::Count); ++i) {
            const StepAxis a = static_cast<StepAxis>(i);
            if (mc_stepper_is_enabled(a))  { en_pins   |= (1U << i); }
            if (mc_stepper_get_dir(a))      { dir_pins  |= (1U << i); }
            if (mc_stepper_step_level(a))   { step_high |= (1U << i); }
        }
        mc_dbg_62_st_en_pins   = en_pins;
        mc_dbg_63_st_dir_pins  = dir_pins;
        mc_dbg_65_st_step_high = step_high;
    }
}

}  // namespace mc
