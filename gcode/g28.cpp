// G28: هومینگ دو مرحله‌ای X و Y، و هومینگ ایمن Z با BLTouch (بند ۶.۳ و ۶.۷).
// G28 بدون پارامتر = X و Y و Z. G28 X Y = فقط همان محورها.
// کار اصلی در core/homing.cpp است.
#include "gcode_handlers.h"
#include "homing.h"

namespace mc {

GcodeStatus mc_gcode_g28(const GcodeCommand& cmd, bool first_call) {
    return mc_homing_run(cmd, first_call);
}

}  // namespace mc
