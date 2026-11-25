// M702 (بند ۹.۲): تخلیه فیلامنت.
#include "gcode_handlers.h"
#include "../core/filament_load.h"

namespace mc {

GcodeStatus mc_gcode_m702(const GcodeCommand& cmd, bool first_call) {
    return mc_fload_run(cmd, first_call, false);
}

}  // namespace mc
