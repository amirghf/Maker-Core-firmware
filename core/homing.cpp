// هومینگ (بند ۶.۳ و ۶.۷): G28 دو مرحله‌ای X و Y، و هومینگ ایمن Z با BLTouch.
// X و Y: (اگر اندستاپ زده است عقب‌نشینی) ← سریع تا اندستاپ ← عقب‌نشینی ← آهسته تا اندستاپ ← موقعیت = صفر.
// Z: بررسی هوم بودن X و Y ← بررسی حدود نقطه امن (100, 60) ← حرکت X سپس Y به نقطه امن ← Deploy پراب ← سریع ← عقب‌نشینی ← آهسته ← موقعیت = -آفست Z ← Stow پین.
#include "homing.h"
#include "../config/mc_config.h"
#include "machine_state.h"
#include "motion_modes.h"
#include "motion_limits.h"
#include "motion_queue.h"
#include "planner.h"
#include "axis_move.h"
#include "stepper_out.h"
#include "hal_step_timer.h"
#include "endstop.h"
#include "endstop_guard.h"
#include "step_gen.h"
#include "error.h"
#include "log.h"
#include "soft_limits.h"
#include "probe.h"
#include "bltouch.h"
#include <math.h>
#include <stdlib.h>

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند. شماره‌ها دائمی‌اند.
extern "C" {
volatile uint32_t mc_dbg_206_hm_state        = 0;    // خواندنی: وضعیت G28 (0 بیکار، 1 در حال کار، 2 تمام شد، 3 خطا)
volatile uint32_t mc_dbg_207_hm_axis         = 255;  // خواندنی: محوری که الان هوم می‌شود (0 X، 1 Y، 2 Z، 255 هیچ)
volatile uint32_t mc_dbg_208_hm_stage        = 0;    // خواندنی: مرحله (0 هیچ، 1 WaitIdle، 2 Release، 3 Fast، 4 BackOff، 5 Slow، 6 SafeX، 7 SafeY، 8 Deploy، 9 Stow، 10 AlarmRelease، 11 StowAgain)
volatile uint32_t mc_dbg_209_hm_result       = 0;    // خواندنی: نتیجه آخرین G28 (0 هیچ، 1 موفق، 2 kill یا arm نیست، 3 فعال‌شدن درایور ناموفق، 4 اندستاپ گیر کرده، 5 اندستاپ پیدا نشد، 6 صف رد کرد، 7 توقف غیرمنتظره، 8 نیاز به هوم X و Y، 9 نقطه امن خارج از حدود، 10 خطای BLTouch، 11 سیگنال قبل Deploy بالا، 12 سیگنال بعد Deploy بالا، 13 سیگنال بعد Stow بالا)
volatile uint32_t mc_dbg_210_hm_homed_mask   = 0;    // خواندنی: ماسک محورهای هوم‌شده (بیت ۰ X، ۱ Y، ۲ Z)
volatile uint32_t mc_dbg_211_hm_runs         = 0;    // خواندنی: تعداد G28 هایی که کامل و موفق تمام شدند
volatile uint32_t mc_dbg_212_hm_cap_mm       = 0;    // نوشتنی: سقف مسافت جست‌وجوی مرحله سریع (میلی‌متر). 0 = مقدار پیش‌فرض
volatile uint32_t mc_dbg_213_hm_fast_um      = 0;    // خواندنی: مسافت طی‌شده در آخرین مرحله سریع (میکرومتر)
volatile uint32_t mc_dbg_214_hm_slow_um      = 0;    // خواندنی: مسافت طی‌شده در آخرین مرحله آهسته (میکرومتر)
volatile int32_t  mc_dbg_249_hm_z_set_um    = 0;    // خواندنی: موقعیت Z که بعد از آخرین هومینگ موفق Z گذاشته شد (میکرومتر)
volatile uint32_t mc_dbg_250_hm_post_stow_hi = 0;   // خواندنی: 1 یعنی در آخرین هومینگ Z بعد از اولین Stow سیگنال پراب هنوز بالا بود

extern volatile uint32_t mc_dbg_80_mv_arm;
extern volatile uint32_t mc_dbg_112_mq_run;
}

namespace mc {
namespace {

// سقف مسافت جست‌وجوی مرحله سریع (میلی‌متر): X و Y و Z (از config/mc_config.h)
constexpr float kHomeMaxMm[3]     = { MC_HOME_MAX_X_MM, MC_HOME_MAX_Y_MM, MC_HOME_MAX_Z_MM };
// سرعت مرحله سریع (میلی‌متر بر ثانیه) (از config/mc_config.h)
constexpr float kHomeFastMmS[3]   = { MC_HOME_FAST_X_MM_S, MC_HOME_FAST_Y_MM_S, MC_HOME_FAST_Z_MM_S };
// سرعت مرحله آهسته و عقب‌نشینی (میلی‌متر بر ثانیه) (از config/mc_config.h)
constexpr float kHomeSlowMmS[3]   = { MC_HOME_SLOW_X_MM_S, MC_HOME_SLOW_Y_MM_S, MC_HOME_SLOW_Z_MM_S };
// فاصله عقب‌نشینی (میلی‌متر) (از config/mc_config.h)
constexpr float kHomeBackoffMm[3] = { MC_HOME_BACKOFF_X_MM, MC_HOME_BACKOFF_Y_MM, MC_HOME_BACKOFF_Z_MM };

// نقطه امن هومینگ Z (میلی‌متر) (از config/mc_config.h)
constexpr float kSafeHomeXMm      = MC_SAFE_HOME_X_MM;
constexpr float kSafeHomeYMm      = MC_SAFE_HOME_Y_MM;

enum class Stage : uint8_t {
    Idle         = 0,
    WaitIdle     = 1,
    Release      = 2,
    Fast         = 3,
    BackOff      = 4,
    Slow         = 5,
    SafeX        = 6,
    SafeY        = 7,
    Deploy       = 8,
    Stow         = 9,
    AlarmRelease = 10,
    StowAgain    = 11
};

constexpr uint32_t kStateIdle  = 0U;
constexpr uint32_t kStateBusy  = 1U;
constexpr uint32_t kStateDone  = 2U;
constexpr uint32_t kStateError = 3U;

constexpr uint32_t kResOk                     = 1U;
constexpr uint32_t kResNotArmed               = 2U;
constexpr uint32_t kResDriver                 = 3U;
constexpr uint32_t kResStuck                  = 4U;
constexpr uint32_t kResNotFound               = 5U;
constexpr uint32_t kResQueue                  = 6U;
constexpr uint32_t kResUnexpected             = 7U;
constexpr uint32_t kResZNeedsXYHomed          = 8U;
constexpr uint32_t kResSafePointOutOfBounds    = 9U;
constexpr uint32_t kResBltouchFail           = 10U;
constexpr uint32_t kResSignalPreDeployHigh   = 11U;
constexpr uint32_t kResSignalPostDeployHigh  = 12U;
constexpr uint32_t kResSignalPostStowHigh    = 13U;

struct Ctx {
    Stage    stage;
    uint8_t  sel_mask;      // محورهای انتخاب‌شده (بیت ۰ X، ۱ Y، ۲ Z)
    uint8_t  axis;          // محور اصلی هومینگ (0 X، 1 Y، 2 Z، 255 هیچ)
    uint8_t  mv_axis;       // محوری که الان برایش گام صادر می‌شود
    bool     forward;       // جهت مرحله جاری (true = مثبت)
    uint32_t total_steps;   // گام کل مرحله جاری
    uint32_t queued_steps;  // گامی که تا الان وارد صف شده
    uint32_t hit_seq;       // شمارنده توقف‌های اندستاپ در شروع مرحله
    float    feed_mm_s;     // سرعت مرحله جاری
    int32_t  start_steps;   // گام واقعی محور در شروع مرحله (برای محاسبه مسافت)
    float    target_safe;   // هدف حرکت نقطه امن
    bool     pin_out;       // آیا پین BLTouch بیرون است؟
};

Ctx     g       = {};
uint8_t g_homed = 0U;   // ماسک محورهای هوم‌شده

uint32_t clamp_u32(long v, uint32_t lo, uint32_t hi) {
    if (v < static_cast<long>(lo)) { return lo; }
    if (v > static_cast<long>(hi)) { return hi; }
    return static_cast<uint32_t>(v);
}

uint32_t mm_to_steps(uint8_t axis, float mm) {
    return clamp_u32(lroundf(mm * mc_mode_spmm_get(axis)), 1U, 100000000U);
}

uint32_t travelled_um(uint8_t axis) {
    const int32_t d  = mc_axis_position_steps(static_cast<StepAxis>(axis)) - g.start_steps;
    const float   mm = static_cast<float>(labs(d)) / mc_mode_spmm_get(axis);
    return static_cast<uint32_t>(lroundf(mm * 1000.0f));
}

// بلوک‌های تک‌محوره را وارد صف می‌کند (از g.mv_axis استفاده می‌کند).
bool push_blocks() {
    const uint8_t a    = g.mv_axis;
    const float   spmm = mc_mode_spmm_get(a);

    float feed = g.feed_mm_s;
    const float vmax = mc_mlim_feed_get(a);
    if (feed > vmax) { feed = vmax; }

    const uint32_t cruise = clamp_u32(lroundf(feed * spmm), 1U, kAxisMoveMaxRateHz);
    uint32_t start = clamp_u32(lroundf(mc_mlim_minfeed_get() * spmm), kStepTimerMinHz, cruise);
    if (start > cruise) { start = cruise; }
    const uint32_t accel = clamp_u32(lroundf(mc_mlim_acc_get(a) * spmm), kAxisAccelMinSps2, kAxisAccelMaxSps2);

    while ((g.queued_steps < g.total_steps) && (mc_plan_count() < (kPlanQueueSize - 1U))) {
        uint32_t n = g.total_steps - g.queued_steps;
        if (n > kAxisMoveMaxSteps) { n = kAxisMoveMaxSteps; }

        MotionBlock b = {};
        b.steps[a]   = n;
        b.start_hz   = start;
        b.cruise_hz  = cruise;
        b.accel_sps2 = accel;
        b.dir_mask   = g.forward ? static_cast<uint8_t>(1U << a) : 0U;

        if (!mc_mq_push(b)) { return false; }
        mc_dbg_112_mq_run = 1U;
        g.queued_steps += n;
    }
    return true;
}

void start_move_axis(Stage s, uint8_t axis, bool forward, uint32_t steps, float feed_mm_s) {
    g.stage        = s;
    g.mv_axis      = axis;
    g.forward      = forward;
    g.total_steps  = steps;
    g.queued_steps = 0U;
    g.feed_mm_s    = feed_mm_s;
    g.hit_seq      = mc_stepgen_endstop_hits();
    g.start_steps  = mc_axis_position_steps(static_cast<StepAxis>(axis));
    mc_dbg_208_hm_stage = static_cast<uint32_t>(s);
}

void start_move(Stage s, bool forward, uint32_t steps, float feed_mm_s) {
    start_move_axis(s, g.axis, forward, steps, feed_mm_s);
}

GcodeStatus fail(uint32_t code, const char* text) {
    mc_mq_clear();
    mc_eguard_resync();
    if (g.pin_out) {
        g.pin_out = false;
        (void)mc_bltouch_start(BltouchCmd::Stow);
    }
    if (g.axis < 3U) {
        g_homed = static_cast<uint8_t>(g_homed & ~(1U << g.axis));
    }
    g.stage = Stage::Idle;
    mc_dbg_208_hm_stage  = 0U;
    mc_dbg_206_hm_state  = kStateError;
    mc_dbg_209_hm_result = code;
    mc_log_value(LogLevel::Error, text, code);
    return GcodeStatus::Error;
}

GcodeStatus start_z_bltouch() {
    if (mc_endstop_triggered(2U)) {
        return fail(kResSignalPreDeployHigh, "homing: probe signal high before deploy");
    }
    if (!mc_motor_enable(StepAxis::Z, true)) {
        return fail(kResDriver, "homing: driver enable failed");
    }
    g.stage = Stage::Deploy;
    mc_dbg_208_hm_stage = static_cast<uint32_t>(Stage::Deploy);
    g.pin_out = true;
    if (!mc_bltouch_start(BltouchCmd::Deploy)) {
        return fail(kResBltouchFail, "homing: bltouch deploy start failed");
    }
    return GcodeStatus::Busy;
}

GcodeStatus begin_next_axis() {
    uint8_t next = (g.axis == 0xFFU) ? 0U : static_cast<uint8_t>(g.axis + 1U);
    while ((next < 3U) && (((g.sel_mask >> next) & 1U) == 0U)) {
        ++next;
    }

    if (next >= 3U) {
        g.axis    = 0xFFU;
        g.mv_axis = 0xFFU;
        g.stage   = Stage::Idle;
        mc_dbg_208_hm_stage  = 0U;
        mc_dbg_206_hm_state  = kStateDone;
        mc_dbg_209_hm_result = kResOk;
        mc_dbg_211_hm_runs   = mc_dbg_211_hm_runs + 1U;
        return GcodeStatus::Done;
    }

    g.axis    = next;
    g.mv_axis = next;
    g_homed   = static_cast<uint8_t>(g_homed & ~(1U << next));

    if (next < 2U) {
        // X یا Y
        if (!mc_motor_enable(static_cast<StepAxis>(next), true)) {
            return fail(kResDriver, "homing: driver enable failed");
        }
        if (mc_endstop_triggered(next)) {
            start_move(Stage::Release, true, mm_to_steps(next, kHomeBackoffMm[next]), kHomeSlowMmS[next]);
        } else {
            float cap_mm = kHomeMaxMm[next];
            const uint32_t ov = mc_dbg_212_hm_cap_mm;
            if ((ov > 0U) && (static_cast<float>(ov) < cap_mm)) {
                cap_mm = static_cast<float>(ov);
            }
            start_move(Stage::Fast, false, mm_to_steps(next, cap_mm), kHomeFastMmS[next]);
        }
        return GcodeStatus::Busy;
    }

    // Z: حرکت به نقطه امن (اول X بعد Y)
    const float cur_x = mc_position_get(0U);
    if (fabsf(kSafeHomeXMm - cur_x) >= 0.01f) {
        if (!mc_motor_enable(StepAxis::X, true)) {
            return fail(kResDriver, "homing: driver enable failed");
        }
        const bool  fwd  = (kSafeHomeXMm > cur_x);
        const float dist = fabsf(kSafeHomeXMm - cur_x);
        g.target_safe    = kSafeHomeXMm;
        start_move_axis(Stage::SafeX, 0U, fwd, mm_to_steps(0U, dist), kHomeFastMmS[0U]);
        return GcodeStatus::Busy;
    }

    const float cur_y = mc_position_get(1U);
    if (fabsf(kSafeHomeYMm - cur_y) >= 0.01f) {
        if (!mc_motor_enable(StepAxis::Y, true)) {
            return fail(kResDriver, "homing: driver enable failed");
        }
        const bool  fwd  = (kSafeHomeYMm > cur_y);
        const float dist = fabsf(kSafeHomeYMm - cur_y);
        g.target_safe    = kSafeHomeYMm;
        start_move_axis(Stage::SafeY, 1U, fwd, mm_to_steps(1U, dist), kHomeFastMmS[1U]);
        return GcodeStatus::Busy;
    }

    return start_z_bltouch();
}

GcodeStatus step_move() {
    const uint8_t a = g.mv_axis;

    if (mc_is_killed() || (mc_dbg_80_mv_arm != 1U)) {
        return fail(kResNotArmed, "homing: kill or arm off");
    }

    const bool busy_before = mc_mq_busy();
    mc_eguard_poll();
    const bool hit      = (mc_stepgen_endstop_hits() != g.hit_seq);
    const bool approach = (g.stage == Stage::Fast) || (g.stage == Stage::Slow);

    if (hit) {
        if (mc_mq_busy()) { return GcodeStatus::Busy; }
        if (!approach) {
            return fail(kResUnexpected, "homing: stop while backing off or moving to safe point");
        }
        if ((mc_stepgen_endstop_last_mask() & (1U << a)) == 0U) {
            return fail(kResUnexpected, "homing: wrong endstop");
        }
        const uint32_t um = travelled_um(a);
        if (g.stage == Stage::Fast) {
            mc_dbg_213_hm_fast_um = um;
            start_move(Stage::BackOff, true, mm_to_steps(a, kHomeBackoffMm[a]), kHomeSlowMmS[a]);
            return GcodeStatus::Busy;
        }

        // مرحله آهسته تمام شد
        mc_dbg_214_hm_slow_um = um;
        if (a < 2U) {
            mc_position_set(a, 0.0f);
            g_homed = static_cast<uint8_t>(g_homed | (1U << a));
            mc_eguard_snapshot();
            return begin_next_axis();
        }

        // Z آهسته تمام شد: موقعیت Z = -آفست Z
        const float z_set = -mc_probe_offset_get(2U);
        mc_position_set(2U, z_set);
        g_homed = static_cast<uint8_t>(g_homed | (1U << 2U));
        mc_eguard_snapshot();
        mc_dbg_249_hm_z_set_um = static_cast<int32_t>(lroundf(z_set * 1000.0f));

        g.stage   = Stage::Stow;
        mc_dbg_208_hm_stage = static_cast<uint32_t>(Stage::Stow);
        g.pin_out = false;
        if (!mc_bltouch_start(BltouchCmd::Stow)) {
            return fail(kResBltouchFail, "homing: bltouch stow start failed");
        }
        return GcodeStatus::Busy;
    }

    if (g.queued_steps < g.total_steps) {
        if (!push_blocks()) {
            return fail(kResQueue, "homing: queue refused");
        }
        return GcodeStatus::Busy;
    }

    if (busy_before) { return GcodeStatus::Busy; }

    // همه گام‌ها بدون زدن اندستاپ انجام شد
    if (approach) {
        return fail(kResNotFound, "homing: endstop not found");
    }

    if (g.stage == Stage::SafeX) {
        mc_position_set(0U, g.target_safe);
        mc_eguard_snapshot();

        const float cur_y = mc_position_get(1U);
        if (fabsf(kSafeHomeYMm - cur_y) >= 0.01f) {
            if (!mc_motor_enable(StepAxis::Y, true)) {
                return fail(kResDriver, "homing: driver enable failed");
            }
            const bool  fwd  = (kSafeHomeYMm > cur_y);
            const float dist = fabsf(kSafeHomeYMm - cur_y);
            g.target_safe    = kSafeHomeYMm;
            start_move_axis(Stage::SafeY, 1U, fwd, mm_to_steps(1U, dist), kHomeFastMmS[1U]);
            return GcodeStatus::Busy;
        }
        return start_z_bltouch();
    }

    if (g.stage == Stage::SafeY) {
        mc_position_set(1U, g.target_safe);
        mc_eguard_snapshot();
        return start_z_bltouch();
    }

    if (mc_endstop_triggered(a)) {
        return fail(kResStuck, "homing: endstop stuck");
    }

    if (g.stage == Stage::Release) {
        float cap_mm = kHomeMaxMm[a];
        const uint32_t ov = mc_dbg_212_hm_cap_mm;
        if ((ov > 0U) && (static_cast<float>(ov) < cap_mm)) {
            cap_mm = static_cast<float>(ov);
        }
        start_move(Stage::Fast, false, mm_to_steps(a, cap_mm), kHomeFastMmS[a]);
    } else {
        // BackOff تمام شد: مرحله آهسته
        start_move(Stage::Slow, false, mm_to_steps(a, kHomeBackoffMm[a] * 2.0f), kHomeSlowMmS[a]);
    }
    return GcodeStatus::Busy;
}

}  // namespace

void mc_homing_init() {
    g       = Ctx{};
    g.axis  = 0xFFU;
    g.mv_axis = 0xFFU;
    g_homed = 0U;
    mc_dbg_206_hm_state        = kStateIdle;
    mc_dbg_207_hm_axis         = 255U;
    mc_dbg_208_hm_stage        = 0U;
    mc_dbg_209_hm_result       = 0U;
    mc_dbg_210_hm_homed_mask   = 0U;
    mc_dbg_211_hm_runs         = 0U;
    mc_dbg_212_hm_cap_mm       = 0U;
    mc_dbg_213_hm_fast_um      = 0U;
    mc_dbg_214_hm_slow_um      = 0U;
    mc_dbg_249_hm_z_set_um     = 0;
    mc_dbg_250_hm_post_stow_hi = 0U;
}

void mc_homing_poll() {
    mc_dbg_207_hm_axis       = g.axis;
    mc_dbg_210_hm_homed_mask = g_homed;
}

GcodeStatus mc_homing_run(const GcodeCommand& cmd, bool first_call) {
    if (first_call) {
        g = Ctx{};
        g.axis  = 0xFFU;
        g.mv_axis = 0xFFU;

        uint8_t sel = 0U;
        if (mc_gcode_has(cmd, 'X')) { sel = static_cast<uint8_t>(sel | 0x1U); }
        if (mc_gcode_has(cmd, 'Y')) { sel = static_cast<uint8_t>(sel | 0x2U); }
        if (mc_gcode_has(cmd, 'Z')) { sel = static_cast<uint8_t>(sel | 0x4U); }
        if (sel == 0U) { sel = 0x7U; }   // بدون پارامتر: هر سه محور
        g.sel_mask = sel;

        g.stage = Stage::WaitIdle;
        mc_dbg_206_hm_state        = kStateBusy;
        mc_dbg_208_hm_stage        = 1U;
        mc_dbg_209_hm_result       = 0U;
        mc_dbg_213_hm_fast_um      = 0U;
        mc_dbg_214_hm_slow_um      = 0U;
        mc_dbg_250_hm_post_stow_hi = 0U;
    }

    mc_eguard_poll();

    switch (g.stage) {
        case Stage::WaitIdle:
            if (mc_mq_busy()) { return GcodeStatus::Busy; }
            if (mc_is_killed() || (mc_dbg_80_mv_arm != 1U)) {
                return fail(kResNotArmed, "homing: kill or arm off");
            }
            if ((g.sel_mask & 0x4U) != 0U) {
                // اگر Z انتخاب شده است: شرط الف (X و Y هوم‌شده باشند) و شرط ب (نقطه امن داخل حدود)
                if (((g_homed | g.sel_mask) & 0x3U) != 0x3U) {
                    return fail(kResZNeedsXYHomed, "homing: z needs x y homed");
                }
                if (!mc_softlimit_allows(0U, kSafeHomeXMm) || !mc_softlimit_allows(1U, kSafeHomeYMm)) {
                    return fail(kResSafePointOutOfBounds, "homing: safe point outside soft limits");
                }
            }
            mc_eguard_snapshot();   // صف خالی است
            return begin_next_axis();

        case Stage::SafeX:
        case Stage::SafeY:
        case Stage::Release:
        case Stage::Fast:
        case Stage::BackOff:
        case Stage::Slow:
            return step_move();

        case Stage::Deploy:
            if (mc_bltouch_busy()) { return GcodeStatus::Busy; }
            if (mc_bltouch_pin_state() != BltouchPin::Deployed) {
                return fail(kResBltouchFail, "homing: bltouch pin not deployed");
            }
            if (mc_endstop_triggered(2U)) {
                return fail(kResSignalPostDeployHigh, "homing: probe signal high after deploy");
            }
            {
                float cap_mm = kHomeMaxMm[2U];
                const uint32_t ov = mc_dbg_212_hm_cap_mm;
                if ((ov > 0U) && (static_cast<float>(ov) < cap_mm)) {
                    cap_mm = static_cast<float>(ov);
                }
                g.mv_axis = 2U;
                start_move(Stage::Fast, false, mm_to_steps(2U, cap_mm), kHomeFastMmS[2U]);
            }
            return GcodeStatus::Busy;

        case Stage::Stow:
            if (mc_bltouch_busy()) { return GcodeStatus::Busy; }
            if (mc_endstop_triggered(2U)) {
                mc_dbg_250_hm_post_stow_hi = 1U;
                g.stage = Stage::AlarmRelease;
                mc_dbg_208_hm_stage = static_cast<uint32_t>(Stage::AlarmRelease);
                if (!mc_bltouch_start(BltouchCmd::AlarmRelease)) {
                    return fail(kResBltouchFail, "homing: bltouch alarm release start failed");
                }
                return GcodeStatus::Busy;
            }
            return begin_next_axis();

        case Stage::AlarmRelease:
            if (mc_bltouch_busy()) { return GcodeStatus::Busy; }
            g.stage = Stage::StowAgain;
            mc_dbg_208_hm_stage = static_cast<uint32_t>(Stage::StowAgain);
            if (!mc_bltouch_start(BltouchCmd::Stow)) {
                return fail(kResBltouchFail, "homing: bltouch stow again start failed");
            }
            return GcodeStatus::Busy;

        case Stage::StowAgain:
            if (mc_bltouch_busy()) { return GcodeStatus::Busy; }
            if (mc_endstop_triggered(2U)) {
                return fail(kResSignalPostStowHigh, "homing: probe signal high after stow");
            }
            return begin_next_axis();

        case Stage::Idle:
        default:
            return fail(kResUnexpected, "homing: bad stage");
    }
}

}  // namespace mc
