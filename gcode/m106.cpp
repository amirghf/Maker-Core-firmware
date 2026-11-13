// M106: روشن‌کردن فن. S<۰ تا ۲۵۵> (بدون S یعنی ۲۵۵). P<شماره فن> (فقط ۰ مجاز است).
#include "gcode_handlers.h"
#include "output_pwm.h"
#include "log.h"

namespace mc {

GcodeStatus mc_gcode_m106(const GcodeCommand& cmd, bool first_call) {
    (void)first_call;
    if (mc_gcode_has_value(cmd, 'P') && (mc_gcode_int(cmd, 'P', 0) != 0)) {
        mc_log(LogLevel::Warn, "M106 invalid fan index");
        return GcodeStatus::Error;
    }
    float s = 255.0f;
    if (mc_gcode_has_value(cmd, 'S')) {
        s = mc_gcode_value(cmd, 'S', 255.0f);
    }
    if (!(s > 0.0f)) { s = 0.0f; }
    if (s > 255.0f) { s = 255.0f; }
    const uint8_t power = static_cast<uint8_t>(s + 0.5f);
    mc_fan_set(power);
    return GcodeStatus::Done;
}

}  // namespace mc
