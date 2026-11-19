// درایور BLTouch (بند ۶.۵). جزئیات در bltouch.h.
#include "bltouch.h"
#include "hal_servo.h"
#include "hal_time.h"
#include "endstop.h"
#include "error.h"

// متغیرهای debug؛ در CubeIDE با Live Expressions دیده می‌شوند. شماره‌ها دائمی‌اند.
extern "C" {
volatile uint32_t mc_dbg_226_bl_busy       = 0;  // خواندنی: ۱ = فرمانی (پالس یا صبر) در جریان است
volatile uint32_t mc_dbg_227_bl_last_angle = 0;  // خواندنی: زاویه آخرین فرمان پذیرفته‌شده (۱۰ و ۹۰ و ۱۲۰ و ۱۶۰). قبل از اولین فرمان ۰
volatile uint32_t mc_dbg_228_bl_pulse_us   = 0;  // خواندنی: عرض پالسی که همین الان روی PA8 داده می‌شود (۰ = بدون پالس)
volatile uint32_t mc_dbg_229_bl_cmd_count  = 0;  // خواندنی: تعداد فرمان‌های پذیرفته‌شده از روشن‌شدن
volatile uint32_t mc_dbg_230_bl_refused    = 0;  // خواندنی: تعداد فرمان‌های ردشده (مشغول، kill، خطای سروو، پارامتر نامعتبر M280)
volatile uint32_t mc_dbg_231_bl_pin_state  = 0;  // خواندنی: وضعیت پین (۰ = نامعلوم، ۱ = بیرون، ۲ = جمع)
volatile uint32_t mc_dbg_234_bl_probe_raw  = 0;  // خواندنی: سطح خام پین سیگنال پراب PC4 (۱ = HIGH)
volatile uint32_t mc_dbg_235_bl_phase      = 0;  // خواندنی: مرحله فعلی (۰ = بیکار، ۱ = ارسال پالس، ۲ = صبر)
}

namespace mc {
namespace {

enum class Phase : uint8_t { Idle = 0, Pulse = 1, Settle = 2 };

struct CmdInfo {
    uint32_t   angle;
    uint32_t   pulse_us;
    uint32_t   settle_ms;
    BltouchPin result;
};

// ترتیب باید با BltouchCmd یکی باشد. عرض پالس‌ها طبق مشخصات عمومی BLTouch است.
constexpr CmdInfo kCmds[] = {
    { 10U,  650U,  kBltouchSettleMs,         BltouchPin::Deployed },
    { 90U,  1475U, kBltouchSettleMs,         BltouchPin::Stowed },
    { 120U, 1780U, kBltouchSelfTestSettleMs, BltouchPin::Unknown },
    { 160U, 2190U, kBltouchAlarmSettleMs,    BltouchPin::Unknown },
};
constexpr uint8_t kCmdCount = static_cast<uint8_t>(sizeof(kCmds) / sizeof(kCmds[0]));

Phase      g_phase          = Phase::Idle;
uint8_t    g_cmd            = 0U;
uint32_t   g_phase_start_ms = 0U;
BltouchPin g_pin            = BltouchPin::Unknown;
uint32_t   g_count          = 0U;
uint32_t   g_refused        = 0U;

void refresh_debug() {
    mc_dbg_226_bl_busy      = (g_phase != Phase::Idle) ? 1U : 0U;
    mc_dbg_228_bl_pulse_us  = mc_servo_pulse_us();
    mc_dbg_229_bl_cmd_count = g_count;
    mc_dbg_230_bl_refused   = g_refused;
    mc_dbg_231_bl_pin_state = static_cast<uint32_t>(g_pin);
    mc_dbg_234_bl_probe_raw = mc_bltouch_probe_raw() ? 1U : 0U;
    mc_dbg_235_bl_phase     = static_cast<uint32_t>(g_phase);
}

}  // namespace

void mc_bltouch_init() {
    g_phase = Phase::Idle;
    g_pin   = BltouchPin::Unknown;
    refresh_debug();
}

bool mc_bltouch_start(BltouchCmd cmd) {
    const uint8_t idx = static_cast<uint8_t>(cmd);
    if ((idx >= kCmdCount) || (g_phase != Phase::Idle) || mc_is_killed()) {
        g_refused = g_refused + 1U;
        refresh_debug();
        return false;
    }
    if (!mc_servo_start(kCmds[idx].pulse_us)) {
        g_refused = g_refused + 1U;
        refresh_debug();
        return false;
    }
    g_cmd            = idx;
    g_phase          = Phase::Pulse;
    g_phase_start_ms = mc_millis();
    g_pin            = BltouchPin::Unknown;   // تا پایان فرمان نامعلوم
    g_count          = g_count + 1U;
    mc_dbg_227_bl_last_angle = kCmds[idx].angle;
    refresh_debug();
    return true;
}

void mc_bltouch_poll() {
    if (mc_is_killed() && (g_phase == Phase::Pulse)) {
        mc_servo_stop();
        g_phase = Phase::Idle;
    }
    if (g_phase == Phase::Pulse) {
        if (mc_time_elapsed_ms(g_phase_start_ms, kBltouchHoldMs)) {
            mc_servo_stop();
            g_phase          = Phase::Settle;
            g_phase_start_ms = mc_millis();
        }
    } else if (g_phase == Phase::Settle) {
        if (mc_time_elapsed_ms(g_phase_start_ms, kCmds[g_cmd].settle_ms)) {
            g_pin   = kCmds[g_cmd].result;
            g_phase = Phase::Idle;
        }
    }
    refresh_debug();
    mc_servo_debug_poll();
}

bool mc_bltouch_busy() {
    return g_phase != Phase::Idle;
}

BltouchPin mc_bltouch_pin_state() {
    return g_pin;
}

bool mc_bltouch_probe_raw() {
    return mc_endstop_raw(2U);
}

bool mc_bltouch_cmd_from_angle(uint32_t angle, BltouchCmd& out) {
    for (uint8_t i = 0U; i < kCmdCount; ++i) {
        if (kCmds[i].angle == angle) {
            out = static_cast<BltouchCmd>(i);
            return true;
        }
    }
    return false;
}

void mc_bltouch_note_refused() {
    g_refused = g_refused + 1U;
    refresh_debug();
}

}  // namespace mc
