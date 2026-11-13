// M109: هدف اکسترودر را می‌گذارد و صبر می‌کند. S<درجه>: فقط گرم‌شدن (بالاتر از هدف هم «رسیده» است).
// R<درجه>: گرم و سردشدن. بدون S و R: کاری نمی‌کند. هدف ۰ (بعد از محدودشدن): هیتر خاموش و فوراً تمام.
// «رسیده» یعنی ۱۰ ثانیه پیوسته داخل پنجره ±۱ درجه (temp_wait.h). وسط صبر هر ۱ ثانیه گزارش دما فرستاده می‌شود.
#include "gcode_handlers.h"
#include "heater_ex.h"
#include "temp_wait.h"
#include "temp_param.h"
#include "log.h"

namespace mc {

GcodeStatus mc_gcode_m109(const GcodeCommand& cmd, bool first_call) {
    if (first_call) {
        if (mc_gcode_has_value(cmd, 'T') && (mc_gcode_int(cmd, 'T', 0) != 0)) {
            mc_log(LogLevel::Warn, "M109 invalid extruder");
            return GcodeStatus::Error;
        }
        if (mc_heater_ex_test_mode()) {
            mc_log(LogLevel::Warn, "M109 refused: heater in debug test mode");
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
        const int32_t got = mc_heater_ex_set_target_x10(mc_temp_c_to_x10(mc_gcode_value(cmd, key, 0.0f)));
        if (got <= 0) { return GcodeStatus::Done; }
        mc_temp_wait_begin(WaitHeater::Extruder, got, cool);
    }
    return mc_temp_wait_poll() ? GcodeStatus::Done : GcodeStatus::Busy;
}

}  // namespace mc
