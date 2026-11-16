// M17: فعال کردن درایور موتورها.
#include "gcode_handlers.h"
#include "motion_cmd.h"
#include "stepper_out.h"

namespace mc {

GcodeStatus mc_gcode_m17(const GcodeCommand& cmd, bool first_call) {
    (void)first_call;
    static const char kLetters[4] = { 'X', 'Y', 'Z', 'E' };
    bool has_any = false;
    for (uint8_t i = 0U; i < 4U; ++i) {
        if (mc_gcode_has(cmd, kLetters[i])) {
            has_any = true;
            break;
        }
    }

    for (uint8_t i = 0U; i < 4U; ++i) {
        if (!has_any || mc_gcode_has(cmd, kLetters[i])) {
            if (!mc_motor_enable(static_cast<StepAxis>(i), true)) {
                mc_gmove_note_mt_refused();
                return GcodeStatus::Error;
            }
        }
    }
    return GcodeStatus::Done;
}

}  // namespace mc
