// M82: E مطلق. (اگر G91 فعال باشد، E باز هم نسبی می‌ماند.)
#include "gcode_handlers.h"
#include "motion_modes.h"

namespace mc {

GcodeStatus mc_gcode_m82(const GcodeCommand& cmd, bool first_call) {
    (void)cmd;
    (void)first_call;
    mc_mode_e_relative_set(false);
    mc_mode_note_applied();
    return GcodeStatus::Done;
}

}  // namespace mc
