// G91: حالت نسبی. مختصات بعدی نسبت به موقعیت فعلی حساب می‌شوند.
#include "gcode_handlers.h"
#include "motion_modes.h"

namespace mc {

GcodeStatus mc_gcode_g91(const GcodeCommand& cmd, bool first_call) {
    (void)cmd;
    (void)first_call;
    mc_mode_relative_set(true);
    mc_mode_note_applied();
    return GcodeStatus::Done;
}

}  // namespace mc
