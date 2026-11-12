// G4: مکث. P<میلی‌ثانیه> یا S<ثانیه> (اگر هر دو بود P). بدون پارامتر: مکث صفر.
// اول صبر می‌کند حرکت‌ها تمام شوند (مثل M400)، بعد زمان را می‌شمارد. حداکثر مکث ۲۴ ساعت.
#include "gcode_handlers.h"
#include "hal_time.h"
#include "machine_state.h"

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_g4_runs = 0;   // تعداد G4 هایی که تمام شدند
volatile uint32_t mc_dbg_g4_ms   = 0;   // مدت مکث آخرین G4 (میلی‌ثانیه، بعد از محدود شدن)
}

namespace mc {
namespace {

constexpr float kMaxDwellMs = 86400000.0f;   // ۲۴ ساعت

}  // namespace

GcodeStatus mc_gcode_g4(const GcodeCommand& cmd, bool first_call) {
    static uint32_t start_ms = 0U;
    static uint32_t wait_ms  = 0U;
    static bool     dwelling = false;   // false = هنوز منتظر تمام شدن حرکت‌ها

    if (first_call) {
        dwelling = false;
        float ms = 0.0f;
        if (mc_gcode_has_value(cmd, 'P')) {
            ms = mc_gcode_value(cmd, 'P', 0.0f);
        } else if (mc_gcode_has_value(cmd, 'S')) {
            ms = mc_gcode_value(cmd, 'S', 0.0f) * 1000.0f;
        }
        if (!(ms > 0.0f)) { ms = 0.0f; }          // منفی یا نامعتبر ← صفر
        if (ms > kMaxDwellMs) { ms = kMaxDwellMs; }
        wait_ms = static_cast<uint32_t>(ms);
        mc_dbg_g4_ms = wait_ms;
    }

    if (!dwelling) {
        if (mc_motion_busy()) { return GcodeStatus::Busy; }
        dwelling = true;
        start_ms = mc_millis();
    }
    if (!mc_time_elapsed_ms(start_ms, wait_ms)) { return GcodeStatus::Busy; }

    mc_dbg_g4_runs = mc_dbg_g4_runs + 1U;
    return GcodeStatus::Done;
}

}  // namespace mc
