// M140: دمای هدف بستر (بدون صبر). S<درجه>. بدون S تغییری نمی‌کند.
// هدف بالاتر از سقف به سقف می‌رسد و هشدار لاگ می‌شود. در حالت تست هیتر (mc_dbg_06_hbed_arm برابر ۱) خطا می‌دهد.
#include "gcode_handlers.h"
#include "heater_bed.h"
#include "temp_param.h"
#include "log.h"

namespace mc {

GcodeStatus mc_gcode_m140(const GcodeCommand& cmd, bool first_call) {
    (void)first_call;
    if (mc_heater_bed_test_mode()) {
        mc_log(LogLevel::Warn, "M140 refused: heater in debug test mode");
        return GcodeStatus::Error;
    }
    if (!mc_gcode_has_value(cmd, 'S')) { return GcodeStatus::Done; }

    const int32_t want = mc_temp_c_to_x10(mc_gcode_value(cmd, 'S', 0.0f));
    const int32_t got  = mc_heater_bed_set_target_x10(want);
    if (want > got) { mc_log_value(LogLevel::Warn, "M140 target clamped (x10)", static_cast<uint32_t>(got)); }
    return GcodeStatus::Done;
}

}  // namespace mc
