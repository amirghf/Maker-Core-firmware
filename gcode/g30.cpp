// G30: اندازه‌گیری ارتفاع بستر با پراب (بند ۶.۶).
#include "gcode_handlers.h"
#include "probe.h"

namespace mc {

GcodeStatus mc_gcode_g30(const GcodeCommand& cmd, bool first_call) {
    return mc_probe_run(cmd, first_call);
}

}  // namespace mc
