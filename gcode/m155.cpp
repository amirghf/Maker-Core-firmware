// M155: گزارش خودکار دما. S<ثانیه>: ۰ = خاموش، حداکثر ۲۵۵ (بیشتر به ۲۵۵ می‌رسد، منفی ۰). بدون S: تغییری نمی‌کند.
#include "gcode_handlers.h"
#include "temp_report.h"

namespace mc {

GcodeStatus mc_gcode_m155(const GcodeCommand& cmd, bool first_call) {
    (void)first_call;
    if (!mc_gcode_has_value(cmd, 'S')) { return GcodeStatus::Done; }
    float s = mc_gcode_value(cmd, 'S', 0.0f);
    if (!(s > 0.0f)) { s = 0.0f; }
    if (s > 255.0f) { s = 255.0f; }
    mc_temp_autoreport_set(static_cast<uint8_t>(s + 0.5f));
    return GcodeStatus::Done;
}

}  // namespace mc
