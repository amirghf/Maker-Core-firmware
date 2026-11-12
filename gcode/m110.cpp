// M110 N<عدد>: شماره آخرین خط را روی عدد می‌گذارد. بدون N کاری نمی‌کند.
// نکته: شکل «N100 M110*34» را بررسی خط (line_check) خودش قبل از رسیدن به اینجا انجام می‌دهد.
#include "gcode_handlers.h"
#include "line_check.h"

namespace mc {

GcodeStatus mc_gcode_m110(const GcodeCommand& cmd, bool first_call) {
    (void)first_call;
    if (mc_gcode_has_value(cmd, 'N')) {
        mc_line_set_last_number(mc_gcode_int(cmd, 'N', 0));
    }
    return GcodeStatus::Done;
}

}  // namespace mc
