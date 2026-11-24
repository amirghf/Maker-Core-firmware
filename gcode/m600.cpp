// M600 (بند ۹.۱): تعویض فیلامنت.
#include "gcode_handlers.h"
#include "../core/filament_change.h"

namespace mc {

GcodeStatus mc_gcode_m600(const GcodeCommand& cmd, bool first_call) {
    return mc_filchg_run(cmd, first_call);
}

}  // namespace mc
