// هومینگ (بند ۶.۳): G28 دو مرحله‌ای.
// هر محور: (اگر اندستاپ زده است عقب‌نشینی) ← سریع تا اندستاپ ← عقب‌نشینی ← آهسته تا اندستاپ ← موقعیت = صفر.
#include "homing.h"
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
#include <math.h>
#include <stdlib.h>

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند. شماره‌ها دائمی‌اند.
extern "C" {
volatile uint32_t mc_dbg_206_hm_state       = 0;    // خواندنی: وضعیت G28 (0 بیکار، 1 در حال کار، 2 تمام شد، 3 خطا)
volatile uint32_t mc_dbg_207_hm_axis        = 255;  // خواندنی: محوری که الان هوم می‌شود (0 X، 1 Y، 2 Z، 255 هیچ)
volatile uint32_t mc_dbg_208_hm_stage       = 0;    // خواندنی: مرحله (0 هیچ، 1 انتظار صف خالی، 2 Release، 3 Fast، 4 BackOff، 5 Slow)
volatile uint32_t mc_dbg_209_hm_result      = 0;    // خواندنی: نتیجه آخرین G28 (0 هیچ، 1 موفق، 2 kill یا arm نیست، 3 فعال‌شدن درایور ناموفق، 4 اندستاپ گیر کرده، 5 اندستاپ پیدا نشد، 6 صف رد کرد، 7 توقف غیرمنتظره)
volatile uint32_t mc_dbg_210_hm_homed_mask  = 0;    // خواندنی: ماسک محورهای هوم‌شده (بیت ۰ X، ۱ Y، ۲ Z)
volatile uint32_t mc_dbg_211_hm_runs        = 0;    // خواندنی: تعداد G28 هایی که کامل و موفق تمام شدند
volatile uint32_t mc_dbg_212_hm_cap_mm      = 0;    // نوشتنی: سقف مسافت جست‌وجوی مرحله سریع (میلی‌متر). 0 = مقدار پیش‌فرض. فقط مقدار کوچک‌تر از پیش‌فرض اثر دارد (برای تست خطا)
volatile uint32_t mc_dbg_213_hm_fast_um     = 0;    // خواندنی: مسافت طی‌شده در آخرین مرحله سریع (میکرومتر)
volatile uint32_t mc_dbg_214_hm_slow_um     = 0;    // خواندنی: مسافت طی‌شده در آخرین مرحله آهسته (میکرومتر)
extern volatile uint32_t mc_dbg_80_mv_arm;
extern volatile uint32_t mc_dbg_112_mq_run;
}

namespace mc {
namespace {

// سقف مسافت جست‌وجوی مرحله سریع (میلی‌متر): X و Y و Z. کمی بیشتر از طول مسیر.
constexpr float kHomeMaxMm[3]     = { 260.0f, 130.0f, 210.0f };
// سرعت مرحله سریع (میلی‌متر بر ثانیه). Y و Z به‌خاطر سقف نرخ گام (kAxisMoveMaxRateHz) کم‌اند.
constexpr float kHomeFastMmS[3]   = { 10.0f, 4.0f, 2.0f };
// سرعت مرحله آهسته و عقب‌نشینی (میلی‌متر بر ثانیه)
constexpr float kHomeSlowMmS[3]   = { 2.0f, 1.0f, 0.5f };
// فاصله عقب‌نشینی (میلی‌متر). سقف جست‌وجوی مرحله آهسته دو برابر همین است.
constexpr float kHomeBackoffMm[3] = { 5.0f, 5.0f, 3.0f };

enum class Stage : uint8_t {
    Idle     = 0,
    WaitIdle = 1,
    Release  = 2,
    Fast     = 3,
    BackOff  = 4,
    Slow     = 5
};

constexpr uint32_t kStateIdle  = 0U;
constexpr uint32_t kStateBusy  = 1U;
constexpr uint32_t kStateDone  = 2U;
constexpr uint32_t kStateError = 3U;

constexpr uint32_t kResOk         = 1U;
constexpr uint32_t kResNotArmed   = 2U;
constexpr uint32_t kResDriver     = 3U;
constexpr uint32_t kResStuck      = 4U;
constexpr uint32_t kResNotFound   = 5U;
constexpr uint32_t kResQueue      = 6U;
constexpr uint32_t kResUnexpected = 7U;

struct Ctx {
    Stage    stage;
    uint8_t  sel_mask;      // محورهای انتخاب‌شده (بیت ۰ X، ۱ Y، ۲ Z)
    uint8_t  axis;          // محور جاری (255 = هنوز شروع نشده یا تمام شده)
    bool     forward;       // جهت مرحله جاری (true = مثبت)
    uint32_t total_steps;   // گام کل مرحله جاری
    uint32_t queued_steps;  // گامی که تا الان وارد صف شده
    uint32_t hit_seq;       // شمارنده توقف‌های اندستاپ در شروع مرحله
    float    feed_mm_s;     // سرعت مرحله جاری
    int32_t  start_steps;   // گام واقعی محور در شروع مرحله (برای محاسبه مسافت)
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

// بلوک‌های تک‌محوره را تا جایی که صف جا دارد وارد صف می‌کند. false = صف بلوک را رد کرد.
bool push_blocks() {
    const uint8_t a    = g.axis;
    const float   spmm = mc_mode_spmm_get(a);

    float feed = g.feed_mm_s;
    const float vmax = mc_mlim_feed_get(a);
    if (feed > vmax) { feed = vmax; }

    const uint32_t cruise = clamp_u32(lroundf(feed * spmm), 1U, kAxisMoveMaxRateHz);
    uint32_t start = clamp_u32(lroundf(mc_mlim_minfeed_get() * spmm), kStepTimerMinHz, cruise);
    if (start > cruise) { start = cruise; }
    const uint32_t accel = clamp_u32(lroundf(mc_mlim_acc_get(2U) * spmm), kAxisAccelMinSps2, kAxisAccelMaxSps2);

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

// یک مرحله حرکتی را آماده می‌کند (بلوک‌ها در تماس‌های بعدی وارد صف می‌شوند).
void start_move(Stage s, bool forward, uint32_t steps, float feed_mm_s) {
    g.stage        = s;
    g.forward      = forward;
    g.total_steps  = steps;
    g.queued_steps = 0U;
    g.feed_mm_s    = feed_mm_s;
    g.hit_seq      = mc_stepgen_endstop_hits();
    g.start_steps  = mc_axis_position_steps(static_cast<StepAxis>(g.axis));
    mc_dbg_208_hm_stage = static_cast<uint32_t>(s);
}

// خطا: حرکت را قطع می‌کند، موقعیت منطقی را از گام واقعی دوباره می‌سازد و Error برمی‌گرداند.
GcodeStatus fail(uint32_t code, const char* text) {
    mc_mq_clear();
    mc_eguard_resync();
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

// محور بعدی از محورهای انتخاب‌شده را شروع می‌کند، یا اگر محوری نمانده G28 را تمام می‌کند.
GcodeStatus begin_next_axis() {
    uint8_t next = (g.axis == 0xFFU) ? 0U : static_cast<uint8_t>(g.axis + 1U);
    while ((next < 3U) && (((g.sel_mask >> next) & 1U) == 0U)) {
        ++next;
    }

    if (next >= 3U) {
        g.axis  = 0xFFU;
        g.stage = Stage::Idle;
        mc_dbg_208_hm_stage  = 0U;
        mc_dbg_206_hm_state  = kStateDone;
        mc_dbg_209_hm_result = kResOk;
        mc_dbg_211_hm_runs   = mc_dbg_211_hm_runs + 1U;
        return GcodeStatus::Done;
    }

    g.axis = next;
    g_homed = static_cast<uint8_t>(g_homed & ~(1U << next));

    if (!mc_motor_enable(static_cast<StepAxis>(next), true)) {
        return fail(kResDriver, "homing: driver enable failed");
    }

    if (mc_endstop_triggered(next)) {
        // اندستاپ از قبل زده است: اول عقب‌نشینی
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

// یک تماس برای مرحله‌های حرکتی (Release، Fast، BackOff، Slow).
GcodeStatus step_move() {
    const uint8_t a = g.axis;

    if (mc_is_killed() || (mc_dbg_80_mv_arm != 1U)) {
        return fail(kResNotArmed, "homing: kill or arm off");
    }

    // ترتیب مهم است: اول busy خوانده می‌شود، بعد توقف اندستاپ پردازش و بررسی می‌شود
    const bool busy_before = mc_mq_busy();
    mc_eguard_poll();
    const bool hit      = (mc_stepgen_endstop_hits() != g.hit_seq);
    const bool approach = (g.stage == Stage::Fast) || (g.stage == Stage::Slow);

    if (hit) {
        if (mc_mq_busy()) { return GcodeStatus::Busy; }   // صف هنوز پاک نشده
        if (!approach) {
            return fail(kResUnexpected, "homing: stop while backing off");
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
        // مرحله آهسته تمام شد: محل سوئیچ = صفر
        mc_dbg_214_hm_slow_um = um;
        mc_position_set(a, 0.0f);
        g_homed = static_cast<uint8_t>(g_homed | (1U << a));
        mc_eguard_snapshot();   // صف خالی است
        return begin_next_axis();
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
        // BackOff تمام شد: مرحله آهسته با سقف دو برابر فاصله عقب‌نشینی
        start_move(Stage::Slow, false, mm_to_steps(a, kHomeBackoffMm[a] * 2.0f), kHomeSlowMmS[a]);
    }
    return GcodeStatus::Busy;
}

}  // namespace

void mc_homing_init() {
    g       = Ctx{};
    g.axis  = 0xFFU;
    g_homed = 0U;
    mc_dbg_206_hm_state      = kStateIdle;
    mc_dbg_207_hm_axis       = 255U;
    mc_dbg_208_hm_stage      = 0U;
    mc_dbg_209_hm_result     = 0U;
    mc_dbg_210_hm_homed_mask = 0U;
    mc_dbg_211_hm_runs       = 0U;
    mc_dbg_212_hm_cap_mm     = 0U;
    mc_dbg_213_hm_fast_um    = 0U;
    mc_dbg_214_hm_slow_um    = 0U;
}

void mc_homing_poll() {
    mc_dbg_207_hm_axis       = g.axis;
    mc_dbg_210_hm_homed_mask = g_homed;
}

GcodeStatus mc_homing_run(const GcodeCommand& cmd, bool first_call) {
    if (first_call) {
        g      = Ctx{};
        g.axis = 0xFFU;

        uint8_t sel = 0U;
        if (mc_gcode_has(cmd, 'X')) { sel = static_cast<uint8_t>(sel | 0x1U); }
        if (mc_gcode_has(cmd, 'Y')) { sel = static_cast<uint8_t>(sel | 0x2U); }
        if (mc_gcode_has(cmd, 'Z')) { sel = static_cast<uint8_t>(sel | 0x4U); }
        if (sel == 0U) { sel = 0x7U; }   // بدون پارامتر: هر سه محور
        g.sel_mask = sel;

        g.stage = Stage::WaitIdle;
        mc_dbg_206_hm_state   = kStateBusy;
        mc_dbg_208_hm_stage   = 1U;
        mc_dbg_209_hm_result  = 0U;
        mc_dbg_213_hm_fast_um = 0U;
        mc_dbg_214_hm_slow_um = 0U;
    }

    mc_eguard_poll();

    switch (g.stage) {
        case Stage::WaitIdle:
            if (mc_mq_busy()) { return GcodeStatus::Busy; }
            if (mc_is_killed() || (mc_dbg_80_mv_arm != 1U)) {
                return fail(kResNotArmed, "homing: kill or arm off");
            }
            mc_eguard_snapshot();   // صف خالی است
            return begin_next_axis();

        case Stage::Release:
        case Stage::Fast:
        case Stage::BackOff:
        case Stage::Slow:
            return step_move();

        case Stage::Idle:
        default:
            return fail(kResUnexpected, "homing: bad stage");
    }
}

}  // namespace mc
