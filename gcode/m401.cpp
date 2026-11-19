// M401: بیرون آوردن پین BLTouch (بند ۶.۵). بدون پارامتر. اول صبر می‌کند حرکت‌ها تمام شوند.
#include "gcode_handlers.h"
#include "bltouch.h"
#include "machine_state.h"

namespace mc {

GcodeStatus mc_gcode_m401(const GcodeCommand& cmd, bool first_call) {
    (void)cmd;
    static bool started = false;
    if (first_call) { started = false; }
    if (!started) {
        if (mc_motion_busy()) { return GcodeStatus::Busy; }
        if (!mc_bltouch_start(BltouchCmd::Deploy)) { return GcodeStatus::Error; }
        started = true;
    }
    if (mc_bltouch_busy()) { return GcodeStatus::Busy; }
    return GcodeStatus::Done;
}

}  // namespace mc
