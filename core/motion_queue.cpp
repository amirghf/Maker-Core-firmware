// پیاده‌سازی صف بلوک‌های حرکت (بند ۵.۶).
// هیچ هدر ST اینجا include نمی‌شود.
#include "motion_queue.h"
#include "ring_buffer.h"
#include "axis_move.h"
#include "hal_step_timer.h"
#include "error.h"

// متغیرهای debug بند ۵.۶ (شماره‌ها دائمی‌اند)
extern "C" {
volatile uint32_t mc_dbg_111_mq_push     = 0;  // برای تست: نوشتنی. با ۱ شدن یک بلوک از متغیرهای mc_dbg_100 تا mc_dbg_104 و mc_dbg_92، mc_dbg_84، mc_dbg_93 ساخته و وارد صف می‌شود و خودش صفر می‌شود.
volatile uint32_t mc_dbg_112_mq_run      = 0;  // برای تست: نوشتنی. با ۱ بودن، بلوک‌های صف اجرا می‌شوند. با ۰ بودن بلوک‌ها در صف می‌مانند.
volatile uint32_t mc_dbg_113_mq_clear    = 0;  // برای تست: نوشتنی. با ۱ شدن بلوک‌های منتظر پاک می‌شوند و خودش صفر می‌شود.
volatile uint32_t mc_dbg_114_mq_count    = 0;  // برای تست: خواندنی. تعداد بلوک‌های منتظر در صف.
volatile uint32_t mc_dbg_115_mq_pushed   = 0;  // برای تست: خواندنی. تعداد کل بلوک‌های پذیرفته‌شده.
volatile uint32_t mc_dbg_116_mq_done     = 0;  // برای تست: خواندنی. تعداد بلوک‌هایی که کامل تمام شدند.
volatile uint32_t mc_dbg_117_mq_refused  = 0;  // برای تست: خواندنی. تعداد بلوک‌های ردشده هنگام ورود.
volatile uint32_t mc_dbg_118_mq_state    = 0;  // برای تست: خواندنی. وضعیت صف (۰ بیکار، ۱ منتظر اجازه، ۲ در حال اجرا، ۳ پاک‌شده).
volatile uint32_t mc_dbg_119_mq_reason   = 0;  // برای تست: خواندنی. دلیل آخرین رد شدن (۱ پر، ۲ گام صفر، ۳ گام زیاد، ۴ نرخ یا شتاب، ۵ kill، ۶ arm).
}

// متغیرهای debug فایل‌های دیگر (تعریف‌شده در axis_move.cpp)
extern "C" {
extern volatile uint32_t mc_dbg_80_mv_arm;
extern volatile uint32_t mc_dbg_84_mv_rate_hz;
extern volatile uint32_t mc_dbg_92_ac_start_hz;
extern volatile uint32_t mc_dbg_93_ac_accel;
extern volatile uint32_t mc_dbg_100_mx_steps_x;
extern volatile uint32_t mc_dbg_101_mx_steps_y;
extern volatile uint32_t mc_dbg_102_mx_steps_z;
extern volatile uint32_t mc_dbg_103_mx_steps_e;
extern volatile uint32_t mc_dbg_104_mx_dirs;
}

namespace mc {

namespace {

RingBuffer<MotionBlock, kMotionQueueSize> g_queue;
bool g_block_running = false;   // آیا بلوک شروع‌شده از صف هنوز در جریان است؟
bool g_cleared       = false;   // آیا صف پاک شده (برای وضعیت ۳)؟

void note_refused(MqReason reason) {
    mc_dbg_117_mq_refused += 1U;
    mc_dbg_119_mq_reason = static_cast<uint32_t>(reason);
}

// بلوک‌های منتظر را پاک می‌کند و وضعیت «پاک‌شده» را ثبت می‌کند
void clear_waiting(bool mark) {
    if (!g_queue.empty() && mark) {
        g_cleared = true;
    }
    g_queue.clear();
}

void update_debug() {
    mc_dbg_114_mq_count = g_queue.size();
    if (g_block_running) {
        mc_dbg_118_mq_state = static_cast<uint32_t>(MqState::Running);
    } else if (!g_queue.empty()) {
        mc_dbg_118_mq_state = (mc_dbg_112_mq_run == 1U)
                                  ? static_cast<uint32_t>(MqState::Running)
                                  : static_cast<uint32_t>(MqState::Waiting);
    } else if (g_cleared) {
        mc_dbg_118_mq_state = static_cast<uint32_t>(MqState::Cleared);
    } else {
        mc_dbg_118_mq_state = static_cast<uint32_t>(MqState::Idle);
    }
}

}  // namespace

void mc_mq_init() {
    g_queue.clear();
    g_block_running = false;
    g_cleared       = false;
    mc_dbg_111_mq_push    = 0U;
    mc_dbg_112_mq_run     = 0U;
    mc_dbg_113_mq_clear   = 0U;
    mc_dbg_114_mq_count   = 0U;
    mc_dbg_115_mq_pushed  = 0U;
    mc_dbg_116_mq_done    = 0U;
    mc_dbg_117_mq_refused = 0U;
    mc_dbg_118_mq_state   = 0U;
    mc_dbg_119_mq_reason  = 0U;
}

bool mc_mq_push(const MotionBlock& block) {
    if (mc_is_killed()) {
        note_refused(MqReason::Killed);
        return false;
    }
    bool any_steps = false;
    for (uint8_t i = 0U; i < 4U; ++i) {
        if (block.steps[i] != 0U) {
            any_steps = true;
        }
        if (block.steps[i] > kAxisMoveMaxSteps) {
            note_refused(MqReason::TooManySteps);
            return false;
        }
    }
    if (!any_steps) {
        note_refused(MqReason::ZeroSteps);
        return false;
    }
    if ((block.start_hz < kStepTimerMinHz) || (block.cruise_hz > kAxisMoveMaxRateHz) ||
        (block.start_hz > block.cruise_hz) ||
        (block.accel_sps2 < kAxisAccelMinSps2) || (block.accel_sps2 > kAxisAccelMaxSps2)) {
        note_refused(MqReason::BadRate);
        return false;
    }
    if (!g_queue.push(block)) {
        note_refused(MqReason::Full);
        return false;
    }
    g_cleared = false;
    mc_dbg_115_mq_pushed += 1U;
    return true;
}

void mc_mq_clear() {
    clear_waiting(true);
}

uint16_t mc_mq_count() {
    return g_queue.size();
}

bool mc_mq_busy() {
    return g_block_running || !g_queue.empty();
}

void mc_mq_poll() {
    // ۱) درخواست پاک کردن صف
    if (mc_dbg_113_mq_clear == 1U) {
        mc_dbg_113_mq_clear = 0U;
        mc_mq_clear();
    }

    // ۲) kill یا arm برابر 1 نیست: صف پاک، درخواست ورود رد
    if (mc_is_killed() || (mc_dbg_80_mv_arm != 1U)) {
        if (mc_dbg_111_mq_push == 1U) {
            mc_dbg_111_mq_push = 0U;
            note_refused(mc_is_killed() ? MqReason::Killed : MqReason::NotArmed);
        }
        if (g_block_running || !g_queue.empty()) {
            g_cleared = true;
        }
        g_queue.clear();
        g_block_running = false;
        mc_dbg_112_mq_run = 0U;
        update_debug();
        return;
    }

    // ۳) درخواست ورود بلوک از متغیرهای debug
    if (mc_dbg_111_mq_push == 1U) {
        mc_dbg_111_mq_push = 0U;
        MotionBlock b;
        b.steps[0]    = mc_dbg_100_mx_steps_x;
        b.steps[1]    = mc_dbg_101_mx_steps_y;
        b.steps[2]    = mc_dbg_102_mx_steps_z;
        b.steps[3]    = mc_dbg_103_mx_steps_e;
        b.dir_mask    = static_cast<uint8_t>(mc_dbg_104_mx_dirs & 0x0FU);
        b.start_hz    = mc_dbg_92_ac_start_hz;
        b.cruise_hz   = mc_dbg_84_mv_rate_hz;
        b.accel_sps2  = mc_dbg_93_ac_accel;
        mc_mq_push(b);
    }

    // ۴) بلوک در حال اجرا: پایان یا قطع آن را بررسی کن
    if (g_block_running) {
        const MoveState st = mc_axis_move_state();
        if (st == MoveState::Moving) {
            update_debug();
            return;
        }
        g_block_running = false;
        if (st == MoveState::Done) {
            mc_dbg_116_mq_done += 1U;
        } else {
            // قطع شد (abort) یا رد شد: بقیه صف بی‌معنی است
            clear_waiting(true);
            g_cleared = true;
            mc_dbg_112_mq_run = 0U;
            update_debug();
            return;
        }
    }

    // ۵) شروع بلوک بعدی (فقط اگر اجرا مجاز است و لایه حرکت بیکار است)
    if ((mc_dbg_112_mq_run == 1U) && !g_queue.empty() && !mc_axis_move_busy()) {
        MotionBlock b;
        if (g_queue.pop(b)) {
            const bool ok = mc_axis_move_start_multi(b.steps, b.dir_mask,
                                                     b.start_hz, b.cruise_hz, b.accel_sps2);
            if (ok) {
                g_block_running = true;
            } else {
                clear_waiting(true);
                g_cleared = true;
                mc_dbg_112_mq_run = 0U;
            }
        }
    }

    update_debug();
}

}  // namespace mc
