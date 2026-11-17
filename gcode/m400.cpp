// M400 (بند ۵.۱۲): صبر می‌کند تا همه حرکت‌های واقعی (بلوک‌های صف و بلوک در حال اجرا) تمام شوند.
// تا آن موقع Busy برمی‌گرداند و ok نمی‌دهد. مدت انتظار برای تست در متغیر debug نوشته می‌شود.
#include "gcode_handlers.h"
#include "hal_time.h"
#include "machine_state.h"

// متغیرهای debug؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_186_m400_runs    = 0;   // خواندنی: تعداد M400 هایی که تمام شدند (بند ۵.۱۲)
volatile uint32_t mc_dbg_187_m400_wait_ms = 0;   // خواندنی: مدت انتظار آخرین M400 از شروع تا تمام شدن (میلی‌ثانیه) (بند ۵.۱۲)
}

namespace mc {
namespace {

uint32_t g_m400_start_ms = 0U;   // لحظه شروع M400 جاری

}  // namespace

GcodeStatus mc_gcode_m400(const GcodeCommand& cmd, bool first_call) {
    (void)cmd;
    if (first_call) { g_m400_start_ms = mc_millis(); }
    if (mc_motion_busy()) { return GcodeStatus::Busy; }
    mc_dbg_187_m400_wait_ms = mc_millis() - g_m400_start_ms;
    mc_dbg_186_m400_runs    = mc_dbg_186_m400_runs + 1U;
    return GcodeStatus::Done;
}

}  // namespace mc
