// G90: حالت مطلق. مختصات بعدی نسبت به مبدأ حساب می‌شوند.
#include "gcode_handlers.h"
#include "motion_modes.h"

namespace mc {

GcodeStatus mc_gcode_g90(const GcodeCommand& cmd, bool first_call) {
    (void)cmd;
    (void)first_call;
    mc_mode_relative_set(false);
    mc_mode_note_applied();
    return GcodeStatus::Done;
}

}  // namespace mc
