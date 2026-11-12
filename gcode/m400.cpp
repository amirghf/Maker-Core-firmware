// M400: صبر می‌کند تا همه حرکت‌ها تمام شوند. (فعلاً حرکتی نداریم؛ مرحله ۵ آن را واقعی می‌کند.)
#include "gcode_handlers.h"
#include "machine_state.h"

namespace mc {

GcodeStatus mc_gcode_m400(const GcodeCommand& cmd, bool first_call) {
    (void)cmd;
    (void)first_call;
    if (mc_motion_busy()) { return GcodeStatus::Busy; }
    return GcodeStatus::Done;
}

}  // namespace mc
