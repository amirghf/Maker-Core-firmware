// پیاده‌سازی تولیدکننده پالس STEP با وقفه تایمر برای یک محور (بند ۵.۲).
// هیچ هدر ST اینجا include نمی‌شود.
#include "step_gen.h"
#include "hal_step_timer.h"
#include "hal_stepper.h"
#include "hal_time.h"
#include "error.h"

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_68_sg_start       = 0;  // برای تست: نوشتنی. ۱ = اجرا را با متغیرهای زیر شروع کن (وظیفه آن را بعد از خواندن به ۰ برمی‌گرداند).
volatile uint32_t mc_dbg_69_sg_steps       = 0;  // برای تست: نوشتنی. تعداد گام اجرا.
volatile uint32_t mc_dbg_70_sg_rate_hz     = 0;  // برای تست: نوشتنی. نرخ شروع (گام در ثانیه).
volatile uint32_t mc_dbg_71_sg_stop        = 0;  // برای تست: نوشتنی. ۱ = اجرا را همین حالا متوقف کن (وظیفه به ۰ برمی‌گرداند).
volatile uint32_t mc_dbg_72_sg_new_rate    = 0;  // برای تست: نوشتنی. عدد غیرصفر = نرخ را حین اجرا به این مقدار عوض کن (وظیفه به ۰ برمی‌گرداند).
volatile uint32_t mc_dbg_73_sg_state       = 0;  // برای تست: خواندنی. ۰ = بیکار، ۱ = در حال اجرا، ۲ = تمام شد، ۳ = متوقف شد (stop دستی یا درایور غیرفعال شد)، ۴ = kill، ۵ = شروع رد شد.
volatile uint32_t mc_dbg_74_sg_remaining   = 0;  // برای تست: خواندنی. گام باقی‌مانده.
volatile uint32_t mc_dbg_75_sg_emitted     = 0;  // برای تست: خواندنی. مجموع گام‌های داده‌شده توسط این تولیدکننده از روشن‌شدن برد.
volatile uint32_t mc_dbg_76_sg_elapsed_ms  = 0;  // برای تست: خواندنی. مدت آخرین اجرای کامل‌شده (میلی‌ثانیه، از شروع تا آخرین گام).
volatile uint32_t mc_dbg_77_sg_isr_max_us  = 0;  // برای تست: خواندنی. طولانی‌ترین زمان اجرای وقفه (میکروثانیه، با mc_micros).
volatile uint32_t mc_dbg_78_sg_refused     = 0;  // برای تست: خواندنی. تعداد شروع‌های ردشده و تغییر نرخ‌های ردشده.
volatile uint32_t mc_dbg_79_sg_actual_hz   = 0;  // برای تست: خواندنی. نرخ واقعی آخرین اجرای کامل‌شده = گام‌ها × ۱۰۰۰ ÷ elapsed_ms (عدد صحیح؛ اگر elapsed صفر بود صفر).
}

// متغیرهای دیباگ از بند ۵.۱ برای خواندن تنظیمات تست
extern "C" {
extern volatile uint32_t mc_dbg_56_st_arm;
extern volatile uint32_t mc_dbg_57_st_axis;
extern volatile uint32_t mc_dbg_59_st_dir;
}

namespace mc {
namespace {

bool     volatile g_running     = false;
StepAxis volatile g_axis        = StepAxis::X;
uint32_t volatile g_remaining   = 0U;
uint32_t volatile g_total_steps = 0U;
uint32_t volatile g_start_ms    = 0U;

void stepgen_isr_callback() {
    const uint32_t t0 = mc_micros();

    // ۲) اگر در حال اجرا نیست: تایمر را متوقف کن و برگرد
    if (!g_running) {
        mc_steptimer_stop();
        return;
    }

    // ۳) اگر برد kill شده است
    if (mc_is_killed()) {
        mc_steptimer_stop();
        g_running = false;
        mc_dbg_73_sg_state = 4U;   // ۴ = kill
        return;
    }

    // ۴) اگر درایور محور غیرفعال است
    if (!mc_stepper_is_enabled(g_axis)) {
        mc_steptimer_stop();
        g_running = false;
        mc_dbg_73_sg_state = 3U;   // ۳ = متوقف شد (درایور غیرفعال شد)
        return;
    }

    // ۵) پالس STEP بده، گام باقی‌مانده یکی کم، پالس‌های صادرشده یکی زیاد
    mc_stepper_pulse(g_axis);
    g_remaining -= 1U;
    mc_dbg_75_sg_emitted += 1U;

    // ۶) اگر گام باقی‌مانده صفر شد
    if (g_remaining == 0U) {
        mc_steptimer_stop();
        g_running = false;
        const uint32_t elapsed = mc_millis() - g_start_ms;
        mc_dbg_76_sg_elapsed_ms = elapsed;
        if (elapsed != 0U) {
            mc_dbg_79_sg_actual_hz = (g_total_steps * 1000U) / elapsed;
        } else {
            mc_dbg_79_sg_actual_hz = 0U;
        }
        mc_dbg_73_sg_state = 2U;   // ۲ = تمام شد
    }

    // ۷) به‌روزرسانی گام باقی‌مانده و محاسبه طولانی‌ترین زمان اجرای وقفه
    mc_dbg_74_sg_remaining = g_remaining;
    const uint32_t isr_us = mc_micros() - t0;
    if (isr_us > mc_dbg_77_sg_isr_max_us) {
        mc_dbg_77_sg_isr_max_us = isr_us;
    }
}

}  // namespace

void mc_stepgen_init() {
    mc_steptimer_set_callback(stepgen_isr_callback);
    g_running     = false;
    g_axis        = StepAxis::X;
    g_remaining   = 0U;
    g_total_steps = 0U;
    g_start_ms    = 0U;

    mc_dbg_68_sg_start      = 0U;
    mc_dbg_69_sg_steps      = 0U;
    mc_dbg_70_sg_rate_hz    = 0U;
    mc_dbg_71_sg_stop       = 0U;
    mc_dbg_72_sg_new_rate   = 0U;
    mc_dbg_73_sg_state      = 0U;   // ۰ = بیکار
    mc_dbg_74_sg_remaining  = 0U;
    mc_dbg_75_sg_emitted    = 0U;
    mc_dbg_76_sg_elapsed_ms = 0U;
    mc_dbg_77_sg_isr_max_us = 0U;
    mc_dbg_78_sg_refused    = 0U;
    mc_dbg_79_sg_actual_hz  = 0U;
}

bool mc_stepgen_start(StepAxis axis, bool forward, uint32_t steps, uint32_t rate_hz) {
    const bool valid_axis = (static_cast<uint8_t>(axis) < static_cast<uint8_t>(StepAxis::Count));
    if (mc_is_killed() || !valid_axis || (steps == 0U) ||
        (rate_hz < kStepTimerMinHz) || (rate_hz > kStepTimerMaxHz) || g_running) {
        mc_dbg_78_sg_refused += 1U;
        mc_dbg_73_sg_state = 5U;   // ۵ = شروع رد شد
        return false;
    }

    if (!mc_stepper_is_enabled(axis)) {
        mc_dbg_78_sg_refused += 1U;
        mc_dbg_73_sg_state = 5U;   // ۵ = شروع رد شد
        return false;
    }

    mc_stepper_set_dir(axis, forward);

    g_axis        = axis;
    g_remaining   = steps;
    g_total_steps = steps;
    g_start_ms    = mc_millis();
    g_running     = true;
    mc_dbg_73_sg_state     = 1U;   // ۱ = در حال اجرا
    mc_dbg_74_sg_remaining = steps;

    if (!mc_steptimer_start(rate_hz)) {
        g_running = false;
        mc_dbg_73_sg_state = 0U;   // بیکار
        mc_dbg_78_sg_refused += 1U;
        return false;
    }

    return true;
}

void mc_stepgen_stop() {
    if (g_running) {
        mc_steptimer_stop();
        g_running = false;
        mc_dbg_73_sg_state = 3U;   // ۳ = متوقف شد
    }
}

bool mc_stepgen_set_rate(uint32_t rate_hz) {
    if (!g_running || !mc_steptimer_set_rate(rate_hz)) {
        mc_dbg_78_sg_refused += 1U;
        return false;
    }
    return true;
}

bool mc_stepgen_busy() {
    return g_running;
}

uint32_t mc_stepgen_remaining() {
    return g_remaining;
}

void mc_stepgen_poll() {
    if (mc_is_killed()) {
        return;
    }

    if (mc_dbg_71_sg_stop == 1U) {
        mc_dbg_71_sg_stop = 0U;
        mc_stepgen_stop();
    }

    if (mc_dbg_72_sg_new_rate != 0U) {
        const uint32_t new_rate = mc_dbg_72_sg_new_rate;
        mc_dbg_72_sg_new_rate = 0U;
        mc_stepgen_set_rate(new_rate);
    }

    if (mc_dbg_68_sg_start == 1U) {
        mc_dbg_68_sg_start = 0U;
        if (mc_dbg_56_st_arm != 1U) {
            mc_dbg_78_sg_refused += 1U;
            mc_dbg_73_sg_state = 5U;   // ۵ = شروع رد شد
        } else if (mc_dbg_57_st_axis > 3U) {
            mc_dbg_78_sg_refused += 1U;
            mc_dbg_73_sg_state = 5U;   // ۵ = شروع رد شد
        } else {
            const StepAxis axis = static_cast<StepAxis>(mc_dbg_57_st_axis);
            const bool fwd = (mc_dbg_59_st_dir != 0U);
            mc_stepgen_start(axis, fwd, mc_dbg_69_sg_steps, mc_dbg_70_sg_rate_hz);
        }
    }

    mc_dbg_74_sg_remaining = g_remaining;
}

}  // namespace mc
