// پیاده‌سازی تولیدکننده پالس STEP با وقفه تایمر برای یک یا چند محور (بند ۵.۲، ۵.۴ و ۵.۵).
// هیچ هدر ST اینجا include نمی‌شود.
#include "step_gen.h"
#include "hal_step_timer.h"
#include "hal_stepper.h"
#include "hal_time.h"
#include "hal_irq.h"
#include "planner.h"
#include "error.h"
#include "endstop.h"   // بند ۶.۲

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_68_sg_start       = 0;  // برای تست: نوشتنی. ۱ = اجرا را با متغیرهای زیر شروع کن (وظیفه آن را بعد از خواندن به ۰ برمی‌گرداند).
volatile uint32_t mc_dbg_69_sg_steps       = 0;  // برای تست: نوشتنی. تعداد گام اجرا.
volatile uint32_t mc_dbg_70_sg_rate_hz     = 0;  // برای تست: نوشتنی. نرخ شروع (گام در ثانیه).
volatile uint32_t mc_dbg_71_sg_stop        = 0;  // برای تست: نوشتنی. ۱ = اجرا را همین حالا متوقف کن (وظیفه به ۰ برمی‌گرداند).
volatile uint32_t mc_dbg_72_sg_new_rate    = 0;  // برای تست: نوشتنی. عدد غیرصفر = نرخ را حین اجرا به این مقدار عوض کن (وظیفه به ۰ برمی‌گرداند).
volatile uint32_t mc_dbg_73_sg_state       = 0;  // برای تست: خواندنی. ۰ = بیکار، ۱ = در حال اجرا، ۲ = تمام شد، ۳ = متوقف شد (stop دستی یا درایور غیرفعال شد)، ۴ = kill، ۵ = شروع رد شد، ۶ = متوقف شد چون اندستاپ زده شد (بند ۶.۲).
volatile uint32_t mc_dbg_74_sg_remaining   = 0;  // برای تست: خواندنی. گام باقی‌مانده.
volatile uint32_t mc_dbg_75_sg_emitted     = 0;  // برای تست: خواندنی. مجموع گام‌های داده‌شده توسط این تولیدکننده از روشن‌شدن برد.
volatile uint32_t mc_dbg_76_sg_elapsed_ms  = 0;  // برای تست: خواندنی. مدت آخرین اجرای کامل‌شده (میلی‌ثانیه، از شروع تا آخرین گام).
volatile uint32_t mc_dbg_77_sg_isr_max_us  = 0;  // برای تست: خواندنی. طولانی‌ترین زمان اجرای وقفه (میکروثانیه، با mc_micros).
volatile uint32_t mc_dbg_78_sg_refused     = 0;  // برای تست: خواندنی. تعداد شروع‌های ردشده و تغییر نرخ‌های ردشده.
volatile uint32_t mc_dbg_79_sg_actual_hz   = 0;  // برای تست: خواندنی. نرخ واقعی آخرین اجرای کامل‌شده = گام‌ها × ۱۰۰۰ ÷ elapsed_ms (عدد صحیح؛ اگر elapsed صفر بود صفر).

volatile uint32_t mc_dbg_95_ac_phase       = 0;  // برای تست: خواندنی. ۰ = بیکار، ۱ = شتاب‌گیری، ۲ = سرعت ثابت، ۳ = کاهش سرعت (بند ۵.۴).
volatile uint32_t mc_dbg_96_ac_accel_steps = 0;  // برای تست: خواندنی. تعداد گام فاز شتاب‌گیری آخرین حرکت دوزنقه‌ای.
volatile uint32_t mc_dbg_97_ac_decel_steps = 0;  // برای تست: خواندنی. تعداد گام فاز کاهش سرعت آخرین حرکت دوزنقه‌ای.
volatile uint32_t mc_dbg_98_ac_peak_hz     = 0;  // برای تست: خواندنی. بیشترین نرخ اعمال‌شده در آخرین حرکت دوزنقه‌ای (در شروع هر حرکت صفر می‌شود).
volatile uint32_t mc_dbg_99_ac_cur_hz      = 0;  // برای تست: خواندنی. آخرین نرخ اعمال‌شده.

volatile uint32_t mc_dbg_106_mx_done_x     = 0;  // برای تست: خواندنی. پالس‌های داده‌شده به X در آخرین حرکت چندمحوره (بند ۵.۵).
volatile uint32_t mc_dbg_107_mx_done_y     = 0;  // برای تست: خواندنی. پالس‌های داده‌شده به Y در آخرین حرکت چندمحوره.
volatile uint32_t mc_dbg_108_mx_done_z     = 0;  // برای تست: خواندنی. پالس‌های داده‌شده به Z در آخرین حرکت چندمحوره.
volatile uint32_t mc_dbg_109_mx_done_e     = 0;  // برای تست: خواندنی. پالس‌های داده‌شده به E در آخرین حرکت چندمحوره.
volatile uint32_t mc_dbg_110_mx_master     = 0;  // برای تست: خواندنی. شماره محور اصلی آخرین حرکت چندمحوره (۰ = X، ۱ = Y، ۲ = Z، ۳ = E).

volatile uint32_t mc_dbg_129_pl_hand_cnt   = 0;  // برای تست: خواندنی. تعداد تعویض بلوک بدون توقف تایمر در آخرین اجرای برنامه‌ریز (بند ۵.۷). در شروع هر اجرا صفر می‌شود.
volatile uint32_t mc_dbg_130_pl_hand_hz    = 0;  // برای تست: خواندنی. نرخ تایمر در آخرین تعویض بلوک (گام بر ثانیه).
volatile uint32_t mc_dbg_131_pl_hand_sel_hz = 0; // برای تست: خواندنی. نرخ تایمر در تعویض شماره mc_dbg_121_pl_sel آخرین اجرا (۰ = تعویض اول). اگر آن تعویض انجام نشده ۰.
volatile uint32_t mc_dbg_132_pl_starts     = 0;  // برای تست: خواندنی. تعداد کل دفعاتی که اجرای برنامه‌ریز تایمر را از ایستاده شروع کرده است.
volatile uint32_t mc_dbg_133_pl_load_max_us = 0; // برای تست: خواندنی. طولانی‌ترین زمان بارگذاری بلوک بعدی داخل وقفه (میکروثانیه).
volatile uint32_t mc_dbg_134_pl_total_ms   = 0;  // برای تست: خواندنی. مدت آخرین اجرای کامل برنامه‌ریز از شروع تا آخرین گام (میلی‌ثانیه).
}

// متغیرهای دیباگ از بند ۵.۱ برای خواندن تنظیمات تست
extern "C" {
extern volatile uint32_t mc_dbg_121_pl_sel;   // تعریف‌شده در planner.cpp (بند ۵.۷)
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

bool     volatile g_ramp       = false;  // آیا اجرای جاری دوزنقه‌ای است؟
uint32_t volatile g_v0_sq      = 0U;     // start_hz به توان ۲
uint32_t volatile g_cruise_hz  = 0U;
uint32_t volatile g_start_hz   = 0U;
uint32_t volatile g_accel      = 0U;
uint32_t volatile g_acc_steps  = 0U;
uint32_t volatile g_dec_steps  = 0U;
uint32_t volatile g_cur_hz     = 0U;

bool     volatile g_multi        = false;  // آیا اجرای جاری چندمحوره است؟ (بند ۵.۵)
uint32_t volatile g_mx_steps[4]  = {0U, 0U, 0U, 0U};  // گام هر محور
uint32_t volatile g_mx_acc[4]    = {0U, 0U, 0U, 0U};  // شمارنده Bresenham هر محور
uint32_t volatile g_mx_emit[4]   = {0U, 0U, 0U, 0U};  // پالس‌های داده‌شده به هر محور
uint8_t  volatile g_mx_master    = 0U;     // شماره محور اصلی
uint32_t volatile g_mx_total     = 0U;     // گام محور اصلی

uint32_t volatile g_exit_sq      = 0U;     // مربع نرخ خروج بلوک جاری (در حرکت‌های قدیمی برابر مربع نرخ شروع) — بند ۵.۷
bool     volatile g_plan         = false;  // آیا اجرای جاری از صف برنامه‌ریز بلوک می‌گیرد؟ (بند ۵.۷)
bool     volatile g_plan_active  = false;  // آیا اجرای برنامه‌ریز در جریان است؟ (برای برنامه‌ریز)
uint32_t volatile g_plan_done    = 0U;     // تعداد بلوک‌هایی که کامل تمام شده‌اند
int32_t  volatile g_fold[4]      = {0, 0, 0, 0};   // جمع گام‌های علامت‌دار بلوک‌های بسته‌شده (برای موقعیت)
bool     volatile g_cur_open     = false;  // آیا بلوک جاری هنوز در g_fold جمع نشده؟
uint32_t volatile g_cur_dirs     = 0U;     // ماسک جهت بلوک جاری
uint32_t volatile g_hand_hz[8]   = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};  // نرخ تایمر در هر تعویض بلوک
uint32_t volatile g_plan_start_ms = 0U;

// بند ۶.۲: توقف به‌خاطر اندستاپ
uint8_t  volatile g_neg_mask = 0U;   // بیت i (۰ = X ، ۱ = Y ، ۲ = Z) = ۱ یعنی در اجرای جاری این محور گام دارد و جهتش منفی (سمت اندستاپ min) است
uint32_t volatile g_eg_hits  = 0U;   // تعداد توقف‌ها به‌خاطر اندستاپ
uint8_t  volatile g_eg_last  = 0U;   // ماسک محورهای آخرین توقف

// ماسک محورهای رو به اندستاپ را از گام و ماسک جهت می‌سازد (فقط X و Y و Z).
uint8_t make_neg_mask(uint32_t sx, uint32_t sy, uint32_t sz, uint8_t dirs) {
    uint8_t m = 0U;
    if ((sx != 0U) && ((dirs & 0x1U) == 0U)) { m = static_cast<uint8_t>(m | 0x1U); }
    if ((sy != 0U) && ((dirs & 0x2U) == 0U)) { m = static_cast<uint8_t>(m | 0x2U); }
    if ((sz != 0U) && ((dirs & 0x4U) == 0U)) { m = static_cast<uint8_t>(m | 0x4U); }
    return m;
}

// ماسک محورهایی که رو به اندستاپ می‌روند و اندستاپشان همین الان زده است (داخل وقفه صدا زده می‌شود؛ باید کوتاه باشد).
uint8_t endstop_hit_now() {
    uint8_t hit = 0U;
    const uint8_t neg = g_neg_mask;
    for (uint8_t i = 0U; i < 3U; ++i) {
        if ((((neg >> i) & 1U) != 0U) && mc_endstop_triggered(i)) {
            hit = static_cast<uint8_t>(hit | (1U << i));
        }
    }
    return hit;
}

// ریشه دوم صحیح (کف) بدون float، به روش بیت‌به‌بیت.
uint32_t isqrt_u32(uint32_t x) {
    uint32_t res = 0U;
    uint32_t bit = 1UL << 30;
    while (bit > x) {
        bit >>= 2;
    }
    while (bit != 0U) {
        if (x >= res + bit) {
            x   -= res + bit;
            res  = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }
    return res;
}

// نرخ گام بعدی را حساب می‌کند و در صورت تغییر به تایمر می‌دهد (داخل وقفه صدا زده می‌شود؛ باید کوتاه باشد).
void ramp_next_rate() {
    const uint32_t k = g_total_steps - g_remaining;   // گام‌های انجام‌شده
    uint32_t rate;
    uint32_t phase;
    if (k < g_acc_steps) {
        rate  = isqrt_u32(g_v0_sq + 2U * g_accel * k);
        phase = 1U;
    } else if (g_remaining <= g_dec_steps) {
        rate  = isqrt_u32(g_exit_sq + 2U * g_accel * (g_remaining - 1U));   // بند ۵.۷: از نرخ خروج
        phase = 3U;
    } else {
        rate  = g_cruise_hz;
        phase = 2U;
    }
    if (rate < g_start_hz)  { rate = g_start_hz; }
    if (rate > g_cruise_hz) { rate = g_cruise_hz; }
    if (rate != g_cur_hz) {
        mc_steptimer_set_rate(rate);
        g_cur_hz = rate;
        mc_dbg_99_ac_cur_hz = rate;
        if (rate > mc_dbg_98_ac_peak_hz) {
            mc_dbg_98_ac_peak_hz = rate;
        }
    }
    mc_dbg_95_ac_phase = phase;
}

// آیا درایور همه محورهای دارای گام فعال است؟ (بند ۵.۵؛ داخل وقفه صدا زده می‌شود)
bool mx_all_enabled() {
    for (uint8_t i = 0U; i < 4U; ++i) {
        if ((g_mx_steps[i] != 0U) && !mc_stepper_is_enabled(static_cast<StepAxis>(i))) {
            return false;
        }
    }
    return true;
}

// در هر وقفه: محور اصلی یک پالس می‌دهد و بقیه محورها طبق Bresenham (فقط عدد صحیح، بدون تقسیم).
void mx_pulse_all() {
    for (uint8_t i = 0U; i < 4U; ++i) {
        if (g_mx_steps[i] == 0U) {
            continue;
        }
        if (i == g_mx_master) {
            mc_stepper_pulse(static_cast<StepAxis>(i));
            g_mx_emit[i] += 1U;
            continue;
        }
        g_mx_acc[i] += g_mx_steps[i];
        if (g_mx_acc[i] >= g_mx_total) {
            g_mx_acc[i] -= g_mx_total;
            mc_stepper_pulse(static_cast<StepAxis>(i));
            g_mx_emit[i] += 1U;
        }
    }
}

// بلوک جاری را در جمع موقعیت می‌ریزد (یک‌بار برای هر بلوک؛ داخل وقفه یا داخل قفل صدا زده می‌شود) — بند ۵.۷
void plan_fold_current() {
    if (!g_cur_open) {
        return;
    }
    for (uint8_t i = 0U; i < 4U; ++i) {
        const int32_t e = static_cast<int32_t>(g_mx_emit[i]);
        g_fold[i] += (((g_cur_dirs >> i) & 1U) != 0U) ? e : -e;
    }
    g_cur_open = false;
}

// همه پرچم‌های اجرای برنامه‌ریز را خاموش می‌کند (پایان، توقف، kill، خطا) — بند ۵.۷
void plan_end_flags() {
    g_plan        = false;
    g_plan_active = false;
}

// بلوک p را به‌عنوان بلوک جاری بارگذاری می‌کند: جهت‌ها، شمارنده‌ها و پروفایل (بند ۵.۷)
void plan_apply(const PlanExec& p) {
    for (uint8_t i = 0U; i < 4U; ++i) {
        g_mx_steps[i] = p.steps[i];
        g_mx_acc[i]   = p.total / 2U;
        g_mx_emit[i]  = 0U;
        if (p.steps[i] != 0U) {
            mc_stepper_set_dir(static_cast<StepAxis>(i), ((p.dir_mask >> i) & 1U) != 0U);
        }
    }
    g_cur_dirs    = p.dir_mask;
    g_neg_mask    = make_neg_mask(p.steps[0], p.steps[1], p.steps[2], p.dir_mask);   // بند ۶.۲
    g_cur_open    = true;
    g_mx_master   = p.master;
    g_mx_total    = p.total;
    g_axis        = static_cast<StepAxis>(p.master);
    g_remaining   = p.total;
    g_total_steps = p.total;
    g_v0_sq       = p.entry_sq;
    g_exit_sq     = p.exit_sq;
    g_cruise_hz   = p.nominal_hz;
    g_accel       = p.accel;
    g_acc_steps   = p.n_acc;
    g_dec_steps   = p.n_dec;
    const uint32_t e_hz = isqrt_u32(p.entry_sq);
    const uint32_t x_hz = isqrt_u32(p.exit_sq);
    uint32_t lo = (e_hz < x_hz) ? e_hz : x_hz;
    if (lo < kStepTimerMinHz) { lo = kStepTimerMinHz; }
    g_start_hz    = lo;
    g_cur_hz      = (e_hz < kStepTimerMinHz) ? kStepTimerMinHz : e_hz;
    if (g_cur_hz > p.nominal_hz) { g_cur_hz = p.nominal_hz; }
    g_ramp        = true;
    g_multi       = true;
    g_start_ms    = mc_millis();
    mc_dbg_110_mx_master     = p.master;
    mc_dbg_96_ac_accel_steps = p.n_acc;
    mc_dbg_97_ac_decel_steps = p.n_dec;
    mc_dbg_98_ac_peak_hz     = g_cur_hz;
    mc_dbg_99_ac_cur_hz      = g_cur_hz;
    mc_dbg_95_ac_phase       = (p.n_acc > 0U) ? 1U : 2U;
    mc_dbg_74_sg_remaining   = p.total;
}

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
        g_ramp = false;
        g_multi = false;               // بند ۵.۵
        plan_fold_current();           // بند ۵.۷
        plan_end_flags();
        mc_dbg_95_ac_phase = 0U;
        mc_dbg_73_sg_state = 4U;   // ۴ = kill
        return;
    }

    // ۴) اگر درایور محور (یا یکی از محورهای حرکت چندمحوره) غیرفعال است
    const bool drivers_ok = g_multi ? mx_all_enabled() : mc_stepper_is_enabled(g_axis);   // بند ۵.۵
    if (!drivers_ok) {
        mc_steptimer_stop();
        g_running = false;
        g_ramp = false;
        g_multi = false;               // بند ۵.۵
        plan_fold_current();           // بند ۵.۷
        plan_end_flags();
        mc_dbg_95_ac_phase = 0U;
        mc_dbg_73_sg_state = 3U;   // ۳ = متوقف شد (درایور غیرفعال شد)
        return;
    }

    // ۴ب) بند ۶.۲: محوری که به سمت اندستاپ min می‌رود و اندستاپش زده شده است: قبل از پالس بعدی همه‌چیز را متوقف کن
    const uint8_t es_hit = endstop_hit_now();
    if (es_hit != 0U) {
        mc_steptimer_stop();
        g_running = false;
        g_ramp = false;
        g_multi = false;
        plan_fold_current();
        plan_end_flags();
        g_eg_last = es_hit;
        g_eg_hits += 1U;
        mc_dbg_95_ac_phase = 0U;
        mc_dbg_73_sg_state = 6U;   // ۶ = متوقف شد چون اندستاپ زده شد
        mc_dbg_74_sg_remaining = g_remaining;
        return;
    }

    // ۵) پالس STEP بده، گام باقی‌مانده یکی کم، پالس‌های صادرشده یکی زیاد
    if (g_multi) {
        mx_pulse_all();                // بند ۵.۵: همه محورها طبق Bresenham
    } else {
        mc_stepper_pulse(g_axis);
    }
    g_remaining -= 1U;
    mc_dbg_75_sg_emitted += 1U;        // فقط گام‌های محور اصلی را می‌شمارد

    // ۶) اگر گام باقی‌مانده صفر شد
    if (g_remaining == 0U) {
        const uint32_t elapsed = mc_millis() - g_start_ms;
        PlanExec nxt;
        bool have_next = false;
        if (g_plan) {                  // بند ۵.۷: بلوک تمام شد؛ بلوک بعدی را بدون توقف تایمر بگیر
            plan_fold_current();
            g_plan_done += 1U;
            have_next = mc_plan_isr_take(nxt);
        }
        if (have_next) {
            const uint32_t l0 = mc_micros();
            plan_apply(nxt);
            mc_steptimer_set_rate(g_cur_hz);
            const uint32_t idx = mc_dbg_129_pl_hand_cnt;
            if (idx < 8U) {
                g_hand_hz[idx] = g_cur_hz;
            }
            mc_dbg_129_pl_hand_cnt = idx + 1U;
            mc_dbg_130_pl_hand_hz  = g_cur_hz;
            const uint32_t load_us = mc_micros() - l0;
            if (load_us > mc_dbg_133_pl_load_max_us) {
                mc_dbg_133_pl_load_max_us = load_us;
            }
        } else {
            mc_steptimer_stop();
            g_running = false;
            g_ramp = false;
            g_multi = false;               // بند ۵.۵
            if (g_plan) {
                plan_end_flags();
                mc_dbg_134_pl_total_ms = mc_millis() - g_plan_start_ms;
            }
            mc_dbg_95_ac_phase = 0U;
            mc_dbg_76_sg_elapsed_ms = elapsed;
            if (elapsed != 0U) {
                mc_dbg_79_sg_actual_hz = (g_total_steps * 1000U) / elapsed;
            } else {
                mc_dbg_79_sg_actual_hz = 0U;
            }
            mc_dbg_73_sg_state = 2U;   // ۲ = تمام شد
        }
    } else if (g_ramp) {
        ramp_next_rate();              // نرخ گام بعدی
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

    g_ramp = false;
    mc_dbg_95_ac_phase       = 0U;
    mc_dbg_96_ac_accel_steps = 0U;
    mc_dbg_97_ac_decel_steps = 0U;
    mc_dbg_98_ac_peak_hz     = 0U;
    mc_dbg_99_ac_cur_hz      = 0U;

    g_multi     = false;
    g_mx_master = 0U;
    g_mx_total  = 0U;
    for (uint8_t i = 0U; i < 4U; ++i) {
        g_mx_steps[i] = 0U;
        g_mx_acc[i]   = 0U;
        g_mx_emit[i]  = 0U;
    }
    mc_dbg_106_mx_done_x = 0U;
    mc_dbg_107_mx_done_y = 0U;
    mc_dbg_108_mx_done_z = 0U;
    mc_dbg_109_mx_done_e = 0U;
    mc_dbg_110_mx_master = 0U;

    g_exit_sq       = 0U;
    g_plan          = false;
    g_plan_active   = false;
    g_plan_done     = 0U;
    g_cur_open      = false;
    g_cur_dirs      = 0U;
    g_plan_start_ms = 0U;
    g_neg_mask      = 0U;
    g_eg_hits       = 0U;
    g_eg_last       = 0U;
    for (uint8_t i = 0U; i < 4U; ++i) { g_fold[i] = 0; }
    for (uint8_t i = 0U; i < 8U; ++i) { g_hand_hz[i] = 0U; }
    mc_dbg_129_pl_hand_cnt    = 0U;
    mc_dbg_130_pl_hand_hz     = 0U;
    mc_dbg_131_pl_hand_sel_hz = 0U;
    mc_dbg_132_pl_starts      = 0U;
    mc_dbg_133_pl_load_max_us = 0U;
    mc_dbg_134_pl_total_ms    = 0U;

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
    g_neg_mask = ((!forward) && (static_cast<uint8_t>(axis) < 3U))
                     ? static_cast<uint8_t>(1U << static_cast<uint8_t>(axis)) : 0U;   // بند ۶.۲

    g_axis        = axis;
    g_remaining   = steps;
    g_total_steps = steps;
    g_start_ms    = mc_millis();
    g_ramp        = false;   // بند ۵.۴: اجرای ثابت‌نرخ
    g_multi       = false;   // بند ۵.۵: اجرای یک‌محوره
    g_plan        = false;   // بند ۵.۷
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

bool mc_stepgen_start_ramp(StepAxis axis, bool forward, uint32_t steps,
                           uint32_t start_hz, uint32_t cruise_hz, uint32_t accel_sps2) {
    const bool valid_axis = (static_cast<uint8_t>(axis) < static_cast<uint8_t>(StepAxis::Count));
    if (mc_is_killed() || !valid_axis || (steps == 0U) ||
        (start_hz < kStepTimerMinHz) || (cruise_hz > kStepTimerMaxHz) || (start_hz > cruise_hz) ||
        (accel_sps2 < kStepAccelMin) || (accel_sps2 > kStepAccelMax) || g_running) {
        mc_dbg_78_sg_refused += 1U;
        mc_dbg_73_sg_state = 5U;   // ۵ = شروع رد شد
        return false;
    }

    if (!mc_stepper_is_enabled(axis)) {
        mc_dbg_78_sg_refused += 1U;
        mc_dbg_73_sg_state = 5U;   // ۵ = شروع رد شد
        return false;
    }

    // تعداد گام شتاب‌گیری: (cruise^2 - start^2) / (2 * accel). بیشترین مقدار cruise^2 = 400,000,000 و در uint32 جا می‌شود.
    const uint32_t n_acc = ((cruise_hz * cruise_hz) - (start_hz * start_hz)) / (2U * accel_sps2);
    uint32_t acc_steps = n_acc;
    uint32_t dec_steps = n_acc;
    if ((2U * n_acc) > steps) {            // حرکت مثلثی: به cruise_hz نمی‌رسد
        acc_steps = steps / 2U;
        dec_steps = steps - acc_steps;
    }

    mc_stepper_set_dir(axis, forward);
    g_neg_mask = ((!forward) && (static_cast<uint8_t>(axis) < 3U))
                     ? static_cast<uint8_t>(1U << static_cast<uint8_t>(axis)) : 0U;   // بند ۶.۲

    g_axis        = axis;
    g_remaining   = steps;
    g_total_steps = steps;
    g_start_hz    = start_hz;
    g_cruise_hz   = cruise_hz;
    g_accel       = accel_sps2;
    g_v0_sq       = start_hz * start_hz;
    g_exit_sq     = g_v0_sq;   // بند ۵.۷: حرکت قدیمی متقارن است
    g_acc_steps   = acc_steps;
    g_dec_steps   = dec_steps;
    g_cur_hz      = start_hz;
    g_start_ms    = mc_millis();
    g_ramp        = true;
    g_multi       = false;   // بند ۵.۵: اجرای یک‌محوره
    g_plan        = false;   // بند ۵.۷
    g_running     = true;

    mc_dbg_96_ac_accel_steps = acc_steps;
    mc_dbg_97_ac_decel_steps = dec_steps;
    mc_dbg_98_ac_peak_hz     = start_hz;
    mc_dbg_99_ac_cur_hz      = start_hz;
    mc_dbg_95_ac_phase       = (acc_steps > 0U) ? 1U : 2U;
    mc_dbg_73_sg_state       = 1U;   // ۱ = در حال اجرا
    mc_dbg_74_sg_remaining   = steps;

    if (!mc_steptimer_start(start_hz)) {
        g_running = false;
        g_ramp = false;
        mc_dbg_95_ac_phase = 0U;
        mc_dbg_73_sg_state = 0U;   // بیکار
        mc_dbg_78_sg_refused += 1U;
        return false;
    }

    return true;
}

bool mc_stepgen_start_multi(const uint32_t* steps, uint8_t dir_mask,
                            uint32_t start_hz, uint32_t cruise_hz, uint32_t accel_sps2) {
    // پیدا کردن محور اصلی (بیشترین گام؛ در تساوی کوچک‌ترین شماره)
    uint8_t master = 0U;
    for (uint8_t i = 1U; i < 4U; ++i) {
        if (steps[i] > steps[master]) {
            master = i;
        }
    }
    const uint32_t total = steps[master];

    if (mc_is_killed() || (total == 0U) ||
        (start_hz < kStepTimerMinHz) || (cruise_hz > kStepTimerMaxHz) || (start_hz > cruise_hz) ||
        (accel_sps2 < kStepAccelMin) || (accel_sps2 > kStepAccelMax) || g_running) {
        mc_dbg_78_sg_refused += 1U;
        mc_dbg_73_sg_state = 5U;   // ۵ = شروع رد شد
        return false;
    }

    // درایور همه محورهای دارای گام باید فعال باشد
    for (uint8_t i = 0U; i < 4U; ++i) {
        if ((steps[i] != 0U) && !mc_stepper_is_enabled(static_cast<StepAxis>(i))) {
            mc_dbg_78_sg_refused += 1U;
            mc_dbg_73_sg_state = 5U;   // ۵ = شروع رد شد
            return false;
        }
    }

    // تعداد گام شتاب‌گیری (برای محور اصلی): همان فرمول بند ۵.۴
    const uint32_t n_acc = ((cruise_hz * cruise_hz) - (start_hz * start_hz)) / (2U * accel_sps2);
    uint32_t acc_steps = n_acc;
    uint32_t dec_steps = n_acc;
    if ((2U * n_acc) > total) {            // حرکت مثلثی
        acc_steps = total / 2U;
        dec_steps = total - acc_steps;
    }

    // جهت و حالت اولیه هر محور
    for (uint8_t i = 0U; i < 4U; ++i) {
        g_mx_steps[i] = steps[i];
        g_mx_acc[i]   = total / 2U;        // شروع از نیمه تا خطای مسیر کمتر شود
        g_mx_emit[i]  = 0U;
        if (steps[i] != 0U) {
            mc_stepper_set_dir(static_cast<StepAxis>(i), ((dir_mask >> i) & 1U) != 0U);
        }
    }
    g_mx_master = master;
    g_mx_total  = total;
    g_neg_mask  = make_neg_mask(steps[0], steps[1], steps[2], dir_mask);   // بند ۶.۲

    g_axis        = static_cast<StepAxis>(master);
    g_remaining   = total;
    g_total_steps = total;
    g_start_hz    = start_hz;
    g_cruise_hz   = cruise_hz;
    g_accel       = accel_sps2;
    g_v0_sq       = start_hz * start_hz;
    g_exit_sq     = g_v0_sq;   // بند ۵.۷: حرکت قدیمی متقارن است
    g_acc_steps   = acc_steps;
    g_dec_steps   = dec_steps;
    g_cur_hz      = start_hz;
    g_start_ms    = mc_millis();
    g_ramp        = true;
    g_multi       = true;
    g_plan        = false;   // بند ۵.۷
    g_cur_open    = false;   // بند ۵.۷: موقعیت این حرکت را axis_move از روی g_mx_emit می‌گیرد
    g_running     = true;

    mc_dbg_110_mx_master     = master;
    mc_dbg_96_ac_accel_steps = acc_steps;
    mc_dbg_97_ac_decel_steps = dec_steps;
    mc_dbg_98_ac_peak_hz     = start_hz;
    mc_dbg_99_ac_cur_hz      = start_hz;
    mc_dbg_95_ac_phase       = (acc_steps > 0U) ? 1U : 2U;
    mc_dbg_73_sg_state       = 1U;   // ۱ = در حال اجرا
    mc_dbg_74_sg_remaining   = total;

    if (!mc_steptimer_start(start_hz)) {
        g_running = false;
        g_ramp = false;
        g_multi = false;
        mc_dbg_95_ac_phase = 0U;
        mc_dbg_73_sg_state = 0U;   // بیکار
        mc_dbg_78_sg_refused += 1U;
        return false;
    }

    return true;
}

uint32_t mc_stepgen_axis_emitted(StepAxis axis) {
    const uint8_t idx = static_cast<uint8_t>(axis);
    if (idx >= 4U) {
        return 0U;
    }
    return g_mx_emit[idx];
}

bool mc_stepgen_plan_start() {
    if (mc_is_killed() || g_running) {
        mc_dbg_78_sg_refused += 1U;
        mc_dbg_73_sg_state = 5U;   // ۵ = شروع رد شد
        return false;
    }
    PlanExec first;
    if (!mc_plan_isr_take(first)) {
        mc_dbg_78_sg_refused += 1U;
        mc_dbg_73_sg_state = 5U;
        return false;
    }
    for (uint8_t i = 0U; i < 4U; ++i) {
        if ((first.steps[i] != 0U) && !mc_stepper_is_enabled(static_cast<StepAxis>(i))) {
            mc_dbg_78_sg_refused += 1U;
            mc_dbg_73_sg_state = 5U;
            return false;
        }
    }
    plan_apply(first);
    g_plan           = true;
    g_plan_active    = true;
    g_plan_start_ms  = mc_millis();
    for (uint8_t i = 0U; i < 8U; ++i) { g_hand_hz[i] = 0U; }
    mc_dbg_129_pl_hand_cnt    = 0U;
    mc_dbg_130_pl_hand_hz     = 0U;
    mc_dbg_131_pl_hand_sel_hz = 0U;
    g_running        = true;
    mc_dbg_73_sg_state = 1U;   // ۱ = در حال اجرا
    if (!mc_steptimer_start(g_cur_hz)) {
        g_running = false;
        g_ramp = false;
        g_multi = false;
        const uint32_t irq = mc_irq_save();
        plan_fold_current();
        plan_end_flags();
        mc_irq_restore(irq);
        mc_dbg_95_ac_phase = 0U;
        mc_dbg_73_sg_state = 0U;
        mc_dbg_78_sg_refused += 1U;
        return false;
    }
    mc_dbg_132_pl_starts += 1U;
    return true;
}

bool mc_stepgen_plan_active() {
    return g_plan_active;
}

uint32_t mc_stepgen_plan_done() {
    return g_plan_done;
}

uint32_t mc_stepgen_block_total() {
    return g_total_steps;
}

int32_t mc_stepgen_plan_delta(StepAxis axis) {
    const uint8_t i = static_cast<uint8_t>(axis);
    if (i >= 4U) {
        return 0;
    }
    const uint32_t irq = mc_irq_save();
    int32_t d = g_fold[i];
    if (g_cur_open) {
        const int32_t e = static_cast<int32_t>(g_mx_emit[i]);
        d += (((g_cur_dirs >> i) & 1U) != 0U) ? e : -e;
    }
    mc_irq_restore(irq);
    return d;
}

int32_t mc_stepgen_plan_take_delta(StepAxis axis) {
    const uint8_t i = static_cast<uint8_t>(axis);
    if ((i >= 4U) || g_running) {
        return 0;
    }
    const uint32_t irq = mc_irq_save();
    const int32_t d = g_fold[i];
    g_fold[i] = 0;
    mc_irq_restore(irq);
    return d;
}

void mc_stepgen_stop() {
    if (g_running) {
        const uint32_t irq = mc_irq_save();   // بند ۵.۷: توقف و بستن بلوک جاری یک‌جا انجام شود
        mc_steptimer_stop();
        g_running = false;
        g_ramp = false;              // بند ۵.۴
        g_multi = false;             // بند ۵.۵
        plan_fold_current();         // بند ۵.۷
        plan_end_flags();
        mc_irq_restore(irq);
        mc_dbg_95_ac_phase = 0U;     // بند ۵.۴
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

uint32_t mc_stepgen_endstop_hits() {
    return g_eg_hits;
}

uint8_t mc_stepgen_endstop_last_mask() {
    return g_eg_last;
}

void mc_stepgen_poll() {
    mc_dbg_106_mx_done_x = g_mx_emit[0];   // بند ۵.۵
    mc_dbg_107_mx_done_y = g_mx_emit[1];
    mc_dbg_108_mx_done_z = g_mx_emit[2];
    mc_dbg_109_mx_done_e = g_mx_emit[3];
    mc_dbg_110_mx_master = g_mx_master;
    {
        const uint32_t sel = mc_dbg_121_pl_sel;
        mc_dbg_131_pl_hand_sel_hz = (sel < 8U) ? g_hand_hz[sel] : 0U;   // بند ۵.۷
    }

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
