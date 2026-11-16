// G0: حرکت سریع خطی.
#include "gcode_handlers.h"
#include "motion_cmd.h"

namespace mc {

GcodeStatus mc_gcode_g0(const GcodeCommand& cmd, bool first_call) {
    return mc_gmove_run(cmd, first_call);
}

}  // namespace mc
