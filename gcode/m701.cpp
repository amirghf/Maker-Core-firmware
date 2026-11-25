// M701 (بند ۹.۲): بارگذاری فیلامنت.
#include "gcode_handlers.h"
#include "../core/filament_load.h"

namespace mc {

GcodeStatus mc_gcode_m701(const GcodeCommand& cmd, bool first_call) {
    return mc_fload_run(cmd, first_call, true);
}

}  // namespace mc
