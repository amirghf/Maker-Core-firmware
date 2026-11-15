// حرکت یک یا چند محور با سرعت ثابت یا دوزنقه‌ای (بند ۵.۳، ۵.۴ و ۵.۵). هیچ هدر ST اینجا include نمی‌شود.
#include "axis_move.h"
#include "step_gen.h"
#include "stepper_out.h"
#include "hal_step_timer.h"
#include "error.h"

// متغیرهای debug؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_80_mv_arm        = 0;  // برای تست: نوشتنی. ۱ = تست حرکت مجاز است. ۰ = همه چیز غیرفعال؛ با صفر شدن، حرکت قطع و موتورها آزاد می‌شوند.
volatile uint32_t mc_dbg_81_mv_axis       = 0;  // برای تست: نوشتنی. محور: ۰ = X، ۱ = Y، ۲ = Z، ۳ = E.
volatile uint32_t mc_dbg_82_mv_dir        = 0;  // برای تست: نوشتنی. جهت: ۱ = مثبت (forward)، ۰ = منفی.
volatile uint32_t mc_dbg_83_mv_steps      = 0;  // برای تست: نوشتنی. تعداد گام حرکت.
volatile uint32_t mc_dbg_84_mv_rate_hz    = 0;  // برای تست: نوشتنی. نرخ (گام در ثانیه).
volatile uint32_t mc_dbg_85_mv_start      = 0;  // برای تست: نوشتنی. با ۱ شدن، حرکت شروع می‌شود و خودش صفر می‌شود.
volatile uint32_t mc_dbg_86_mv_abort      = 0;  // برای تست: نوشتنی. با ۱ شدن، حرکت قطع می‌شود و خودش صفر می‌شود.
volatile uint32_t mc_dbg_87_mv_state      = 0;  // برای تست: خواندنی. ۰ بیکار، ۱ در حال حرکت، ۲ تمام شد، ۳ قطع شد، ۴ رد شد.
volatile int32_t  mc_dbg_88_mv_pos_steps  = 0;  // برای تست: خواندنی. موقعیت محور انتخاب‌شده بر حسب گام (با علامت).
volatile uint32_t mc_dbg_89_mv_refused    = 0;  // برای تست: خواندنی. تعداد درخواست‌های ردشده.
volatile uint32_t mc_dbg_90_mv_reason     = 0;  // برای تست: خواندنی. دلیل آخرین رد شدن (۱ kill، ۲ محور، ۳ گام صفر، ۴ گام زیاد، ۵ نرخ، ۶ مشغول، ۷ arm، ۸ شروع نشد، ۹ شتاب).
volatile uint32_t mc_dbg_91_mv_done_steps = 0;  // برای تست: خواندنی. گام‌های انجام‌شده در آخرین حرکت (تمام یا قطع‌شده).

volatile uint32_t mc_dbg_92_ac_start_hz = 0;  // برای تست: نوشتنی. نرخ شروع و پایان حرکت دوزنقه‌ای (گام بر ثانیه) — بند ۵.۴.
volatile uint32_t mc_dbg_93_ac_accel    = 0;  // برای تست: نوشتنی. شتاب (گام بر ثانیه‌مربع).
volatile uint32_t mc_dbg_94_ac_go       = 0;  // برای تست: نوشتنی. با ۱ شدن حرکت دوزنقه‌ای شروع می‌شود و خودش صفر می‌شود. نرخ سرعت ثابت از mc_dbg_84_mv_rate_hz خوانده می‌شود.

volatile uint32_t mc_dbg_100_mx_steps_x = 0;  // برای تست: نوشتنی. گام X در حرکت چندمحوره (بند ۵.۵).
volatile uint32_t mc_dbg_101_mx_steps_y = 0;  // برای تست: نوشتنی. گام Y در حرکت چندمحوره.
volatile uint32_t mc_dbg_102_mx_steps_z = 0;  // برای تست: نوشتنی. گام Z در حرکت چندمحوره.
volatile uint32_t mc_dbg_103_mx_steps_e = 0;  // برای تست: نوشتنی. گام E در حرکت چندمحوره.
volatile uint32_t mc_dbg_104_mx_dirs    = 0;  // برای تست: نوشتنی. ماسک جهت: بیت ۰ = X، بیت ۱ = Y، بیت ۲ = Z، بیت ۳ = E؛ ۱ = مثبت. (مثلاً ۳ یعنی X و Y مثبت، Z و E منفی)
volatile uint32_t mc_dbg_105_mx_go      = 0;  // برای تست: نوشتنی. با ۱ شدن حرکت چندمحوره شروع می‌شود و خودش صفر می‌شود. نرخ محور اصلی از mc_dbg_84_mv_rate_hz، نرخ شروع از mc_dbg_92_ac_start_hz و شتاب از mc_dbg_93_ac_accel خوانده می‌شود.
}

namespace mc {
namespace {

MoveState g_state      = MoveState::Idle;
StepAxis  g_axis       = StepAxis::X;
bool      g_forward    = true;
uint32_t  g_total      = 0U;
int32_t   g_pos[4]     = {0, 0, 0, 0};
bool      g_prev_armed = false;

bool      g_multi_move = false;   // آیا حرکت جاری چندمحوره است؟ (بند ۵.۵)
uint8_t   g_dir_mask   = 0U;      // ماسک جهت حرکت چندمحوره (بند ۵.۵)

// یک درخواست ردشده را ثبت می‌کند. اگر حرکتی در جریان است، وضعیت آن را به Refused تغییر نمی‌دهد.
void note_refused(MoveRefuse why) {
    mc_dbg_89_mv_refused = mc_dbg_89_mv_refused + 1U;
    mc_dbg_90_mv_reason  = static_cast<uint32_t>(why);
    if (g_state != MoveState::Moving) {
        g_state = MoveState::Refused;
    }
}

// حرکت جاری تمام شده یا قطع شده است؛ گام‌های انجام‌شده را حساب و موقعیت را به‌روز می‌کند.
void close_move() {
    const uint32_t left  = mc_stepgen_remaining();
    const uint32_t done  = (left <= g_total) ? (g_total - left) : 0U;
    if (g_multi_move) {
        // بند ۵.۵: موقعیت هر محور از روی پالس‌های واقعی همان محور
        for (uint8_t i = 0U; i < 4U; ++i) {
            const int32_t e = static_cast<int32_t>(mc_stepgen_axis_emitted(static_cast<StepAxis>(i)));
            const bool fwd  = (((g_dir_mask >> i) & 1U) != 0U);
            g_pos[i] += fwd ? e : -e;
        }
    } else {
        const int32_t  delta = static_cast<int32_t>(done);
        const uint8_t  idx   = static_cast<uint8_t>(g_axis);
        g_pos[idx] += g_forward ? delta : -delta;
    }
    mc_dbg_91_mv_done_steps = done;    // گام‌های انجام‌شده محور اصلی
    g_state = (left == 0U) ? MoveState::Done : MoveState::Aborted;
}

// متغیرهای خواندنی debug را به‌روز می‌کند.
void update_debug() {
    mc_dbg_87_mv_state = static_cast<uint32_t>(g_state);
    const uint32_t sel = mc_dbg_81_mv_axis;
    mc_dbg_88_mv_pos_steps = (sel < 4U) ? g_pos[sel] : 0;
}

}  // namespace

void mc_axis_move_init() {
    g_state      = MoveState::Idle;
    g_axis       = StepAxis::X;
    g_forward    = true;
    g_total      = 0U;
    g_prev_armed = false;
    for (uint8_t i = 0U; i < 4U; ++i) {
        g_pos[i] = 0;
    }
    mc_dbg_80_mv_arm        = 0U;
    mc_dbg_81_mv_axis       = 0U;
    mc_dbg_82_mv_dir        = 0U;
    mc_dbg_83_mv_steps      = 0U;
    mc_dbg_84_mv_rate_hz    = 0U;
    mc_dbg_85_mv_start      = 0U;
    mc_dbg_86_mv_abort      = 0U;
    mc_dbg_87_mv_state      = 0U;
    mc_dbg_88_mv_pos_steps  = 0;
    mc_dbg_89_mv_refused    = 0U;
    mc_dbg_90_mv_reason     = 0U;
    mc_dbg_91_mv_done_steps = 0U;

    mc_dbg_92_ac_start_hz   = 0U;
    mc_dbg_93_ac_accel      = 0U;
    mc_dbg_94_ac_go         = 0U;

    g_multi_move = false;
    g_dir_mask   = 0U;
    mc_dbg_100_mx_steps_x = 0U;
    mc_dbg_101_mx_steps_y = 0U;
    mc_dbg_102_mx_steps_z = 0U;
    mc_dbg_103_mx_steps_e = 0U;
    mc_dbg_104_mx_dirs    = 0U;
    mc_dbg_105_mx_go      = 0U;
}

bool mc_axis_move_start(StepAxis axis, bool forward, uint32_t steps, uint32_t rate_hz) {
    const uint8_t idx = static_cast<uint8_t>(axis);

    if (mc_is_killed()) {
        note_refused(MoveRefuse::Killed);
        return false;
    }
    if (idx >= static_cast<uint8_t>(StepAxis::Count)) {
        note_refused(MoveRefuse::BadAxis);
        return false;
    }
    if (steps == 0U) {
        note_refused(MoveRefuse::ZeroSteps);
        return false;
    }
    if (steps > kAxisMoveMaxSteps) {
        note_refused(MoveRefuse::TooManySteps);
        return false;
    }
    if ((rate_hz < kStepTimerMinHz) || (rate_hz > kAxisMoveMaxRateHz)) {
        note_refused(MoveRefuse::BadRate);
        return false;
    }
    if ((g_state == MoveState::Moving) || mc_stepgen_busy()) {
        note_refused(MoveRefuse::Busy);
        return false;
    }
    if (!mc_motor_enable(axis, true)) {
        note_refused(MoveRefuse::Killed);
        return false;
    }

    g_axis       = axis;
    g_forward    = forward;
    g_total      = steps;
    g_multi_move = false;   // بند ۵.۵: حرکت یک‌محوره
    if (!mc_stepgen_start(axis, forward, steps, rate_hz)) {
        note_refused(MoveRefuse::StartFailed);
        return false;
    }

    g_state = MoveState::Moving;
    mc_dbg_91_mv_done_steps = 0U;
    return true;
}

bool mc_axis_move_start_ramp(StepAxis axis, bool forward, uint32_t steps,
                             uint32_t start_hz, uint32_t cruise_hz, uint32_t accel_sps2) {
    const uint8_t idx = static_cast<uint8_t>(axis);

    if (mc_is_killed()) {
        note_refused(MoveRefuse::Killed);
        return false;
    }
    if (idx >= static_cast<uint8_t>(StepAxis::Count)) {
        note_refused(MoveRefuse::BadAxis);
        return false;
    }
    if (steps == 0U) {
        note_refused(MoveRefuse::ZeroSteps);
        return false;
    }
    if (steps > kAxisMoveMaxSteps) {
        note_refused(MoveRefuse::TooManySteps);
        return false;
    }
    if ((start_hz < kStepTimerMinHz) || (cruise_hz > kAxisMoveMaxRateHz) || (start_hz > cruise_hz)) {
        note_refused(MoveRefuse::BadRate);
        return false;
    }
    if ((accel_sps2 < kAxisAccelMinSps2) || (accel_sps2 > kAxisAccelMaxSps2)) {
        note_refused(MoveRefuse::BadAccel);
        return false;
    }
    if ((g_state == MoveState::Moving) || mc_stepgen_busy()) {
        note_refused(MoveRefuse::Busy);
        return false;
    }
    if (!mc_motor_enable(axis, true)) {
        note_refused(MoveRefuse::Killed);
        return false;
    }

    g_axis       = axis;
    g_forward    = forward;
    g_total      = steps;
    g_multi_move = false;   // بند ۵.۵: حرکت یک‌محوره
    if (!mc_stepgen_start_ramp(axis, forward, steps, start_hz, cruise_hz, accel_sps2)) {
        note_refused(MoveRefuse::StartFailed);
        return false;
    }

    g_state = MoveState::Moving;
    mc_dbg_91_mv_done_steps = 0U;
    return true;
}

bool mc_axis_move_start_multi(const uint32_t* steps, uint8_t dir_mask,
                              uint32_t start_hz, uint32_t cruise_hz, uint32_t accel_sps2) {
    if (mc_is_killed()) {
        note_refused(MoveRefuse::Killed);
        return false;
    }

    uint32_t total = 0U;
    for (uint8_t i = 0U; i < 4U; ++i) {
        if (steps[i] > kAxisMoveMaxSteps) {
            note_refused(MoveRefuse::TooManySteps);
            return false;
        }
        if (steps[i] > total) {
            total = steps[i];
        }
    }
    if (total == 0U) {
        note_refused(MoveRefuse::ZeroSteps);
        return false;
    }
    if ((start_hz < kStepTimerMinHz) || (cruise_hz > kAxisMoveMaxRateHz) || (start_hz > cruise_hz)) {
        note_refused(MoveRefuse::BadRate);
        return false;
    }
    if ((accel_sps2 < kAxisAccelMinSps2) || (accel_sps2 > kAxisAccelMaxSps2)) {
        note_refused(MoveRefuse::BadAccel);
        return false;
    }
    if ((g_state == MoveState::Moving) || mc_stepgen_busy()) {
        note_refused(MoveRefuse::Busy);
        return false;
    }
    // فقط درایور محورهای دارای گام فعال شود
    for (uint8_t i = 0U; i < 4U; ++i) {
        if ((steps[i] != 0U) && !mc_motor_enable(static_cast<StepAxis>(i), true)) {
            mc_motor_disable_all();
            note_refused(MoveRefuse::Killed);
            return false;
        }
    }

    g_total      = total;
    g_dir_mask   = static_cast<uint8_t>(dir_mask & 0x0FU);
    g_multi_move = true;
    if (!mc_stepgen_start_multi(steps, g_dir_mask, start_hz, cruise_hz, accel_sps2)) {
        g_multi_move = false;
        note_refused(MoveRefuse::StartFailed);
        return false;
    }

    g_state = MoveState::Moving;
    mc_dbg_91_mv_done_steps = 0U;
    return true;
}

void mc_axis_move_abort() {
    if (g_state != MoveState::Moving) {
        return;
    }
    mc_stepgen_stop();
    close_move();
}

bool mc_axis_move_busy() {
    return g_state == MoveState::Moving;
}

int32_t mc_axis_position_steps(StepAxis axis) {
    const uint8_t idx = static_cast<uint8_t>(axis);
    if (idx >= static_cast<uint8_t>(StepAxis::Count)) {
        return 0;
    }
    return g_pos[idx];
}

MoveState mc_axis_move_state() {
    return g_state;
}

void mc_axis_move_poll() {
    // ۱) اگر arm برابر ۱ نیست: حرکت قطع، موتورها آزاد (فقط وقتی تازه از ۱ به غیر ۱ رفته)، درخواست‌ها رد
    if (mc_dbg_80_mv_arm != 1U) {
        if (g_state == MoveState::Moving) {
            mc_axis_move_abort();
        }
        if (g_prev_armed) {
            mc_motor_disable_all();
        }
        if (mc_dbg_85_mv_start == 1U) {
            mc_dbg_85_mv_start = 0U;
            note_refused(MoveRefuse::NotArmed);
        }
        if (mc_dbg_94_ac_go == 1U) {
            mc_dbg_94_ac_go = 0U;
            note_refused(MoveRefuse::NotArmed);
        }
        if (mc_dbg_105_mx_go == 1U) {
            mc_dbg_105_mx_go = 0U;
            note_refused(MoveRefuse::NotArmed);
        }
        mc_dbg_86_mv_abort = 0U;
        g_prev_armed = false;
        update_debug();
        return;
    }
    g_prev_armed = true;

    // ۲) اگر برد kill شده: حرکت قطع و درخواست‌ها پاک
    if (mc_is_killed()) {
        if (g_state == MoveState::Moving) {
            mc_axis_move_abort();
        }
        mc_dbg_85_mv_start = 0U;
        mc_dbg_94_ac_go    = 0U;
        mc_dbg_105_mx_go   = 0U;
        mc_dbg_86_mv_abort = 0U;
        update_debug();
        return;
    }

    // ۳) درخواست قطع
    if (mc_dbg_86_mv_abort == 1U) {
        mc_dbg_86_mv_abort = 0U;
        mc_axis_move_abort();
    }

    // ۴) تشخیص پایان حرکت (مولد گام دیگر مشغول نیست)
    if ((g_state == MoveState::Moving) && !mc_stepgen_busy()) {
        close_move();
    }

    // ۵) درخواست شروع
    if (mc_dbg_85_mv_start == 1U) {
        mc_dbg_85_mv_start = 0U;
        if (mc_dbg_81_mv_axis > 3U) {
            note_refused(MoveRefuse::BadAxis);
        } else {
            mc_axis_move_start(static_cast<StepAxis>(mc_dbg_81_mv_axis),
                               (mc_dbg_82_mv_dir != 0U),
                               mc_dbg_83_mv_steps,
                               mc_dbg_84_mv_rate_hz);
        }
    }

    // ۶) درخواست شروع حرکت دوزنقه‌ای (بند ۵.۴)
    if (mc_dbg_94_ac_go == 1U) {
        mc_dbg_94_ac_go = 0U;
        if (mc_dbg_81_mv_axis > 3U) {
            note_refused(MoveRefuse::BadAxis);
        } else {
            mc_axis_move_start_ramp(static_cast<StepAxis>(mc_dbg_81_mv_axis),
                                    (mc_dbg_82_mv_dir != 0U),
                                    mc_dbg_83_mv_steps,
                                    mc_dbg_92_ac_start_hz,
                                    mc_dbg_84_mv_rate_hz,
                                    mc_dbg_93_ac_accel);
        }
    }

    // ۷) درخواست شروع حرکت چندمحوره (بند ۵.۵)
    if (mc_dbg_105_mx_go == 1U) {
        mc_dbg_105_mx_go = 0U;
        const uint32_t mx_steps[4] = {mc_dbg_100_mx_steps_x, mc_dbg_101_mx_steps_y,
                                      mc_dbg_102_mx_steps_z, mc_dbg_103_mx_steps_e};
        mc_axis_move_start_multi(mx_steps,
                                 static_cast<uint8_t>(mc_dbg_104_mx_dirs & 0x0FU),
                                 mc_dbg_92_ac_start_hz,
                                 mc_dbg_84_mv_rate_hz,
                                 mc_dbg_93_ac_accel);
    }

    update_debug();
}

}  // namespace mc
