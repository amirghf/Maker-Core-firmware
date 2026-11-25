// M412 (بند ۹.۳): تنظیم یا گزارش وضعیت سنسور اتمام فیلامنت.
#include "gcode_handlers.h"
#include "../core/filament_sensor.h"
#include "../core/host_reply.h"

namespace mc {

GcodeStatus mc_gcode_m412(const GcodeCommand& cmd, bool first_call) {
    (void)first_call;

    if (mc_gcode_has(cmd, 'S')) {
        if (!mc_gcode_has_value(cmd, 'S')) {
            return GcodeStatus::Error;
        }
        const int32_t val = mc_gcode_int(cmd, 'S', -1);
        if ((val != 0) && (val != 1)) {
            return GcodeStatus::Error;
        }
        mc_dbg_456_fil_run_en = static_cast<uint32_t>(val);
        return GcodeStatus::Done;
    }

    const bool en = mc_filament_runout_enabled();
    mc_reply_line(en ? "echo:Filament runout: ON" : "echo:Filament runout: OFF");
    return GcodeStatus::Done;
}

}  // namespace mc
