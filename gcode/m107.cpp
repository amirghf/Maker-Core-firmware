// M107: خاموش‌کردن فن. P<شماره فن> (فقط ۰ مجاز است).
#include "gcode_handlers.h"
#include "output_pwm.h"
#include "log.h"

namespace mc {

GcodeStatus mc_gcode_m107(const GcodeCommand& cmd, bool first_call) {
    (void)first_call;
    if (mc_gcode_has_value(cmd, 'P') && (mc_gcode_int(cmd, 'P', 0) != 0)) {
        mc_log(LogLevel::Warn, "M107 invalid fan index");
        return GcodeStatus::Error;
    }
    mc_fan_set(0);
    return GcodeStatus::Done;
}

}  // namespace mc
