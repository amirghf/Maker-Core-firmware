// M190: هدف بستر را می‌گذارد و صبر می‌کند. S<درجه>: فقط گرم‌شدن.
// R<درجه>: گرم و سردشدن. بدون S و R: کاری نمی‌کند. هدف ۰: هیتر خاموش و فوراً تمام.
#include "gcode_handlers.h"
#include "heater_bed.h"
#include "temp_wait.h"
#include "temp_param.h"
#include "log.h"

namespace mc {

GcodeStatus mc_gcode_m190(const GcodeCommand& cmd, bool first_call) {
    if (first_call) {
        if (mc_heater_bed_test_mode()) {
            mc_log(LogLevel::Warn, "M190 refused: heater in debug test mode");
            return GcodeStatus::Error;
        }
        bool cool = false;
        char key = 'S';
        if (mc_gcode_has_value(cmd, 'S')) {
            key = 'S';
        } else if (mc_gcode_has_value(cmd, 'R')) {
            key = 'R';
            cool = true;
        } else {
            return GcodeStatus::Done;
        }
        const int32_t got = mc_heater_bed_set_target_x10(mc_temp_c_to_x10(mc_gcode_value(cmd, key, 0.0f)));
        if (got <= 0) { return GcodeStatus::Done; }
        mc_temp_wait_begin(WaitHeater::Bed, got, cool);
    }
    return mc_temp_wait_poll() ? GcodeStatus::Done : GcodeStatus::Busy;
}

}  // namespace mc
