// M83: E نسبی.
#include "gcode_handlers.h"
#include "motion_modes.h"

namespace mc {

GcodeStatus mc_gcode_m83(const GcodeCommand& cmd, bool first_call) {
    (void)cmd;
    (void)first_call;
    mc_mode_e_relative_set(true);
    mc_mode_note_applied();
    return GcodeStatus::Done;
}

}  // namespace mc
