// M84: غیرفعال کردن موتورها (همان رفتار M18).
#include "gcode_handlers.h"
#include "motion_cmd.h"
#include "motion_queue.h"
#include "stepper_out.h"

namespace mc {

GcodeStatus mc_gcode_m84(const GcodeCommand& cmd, bool first_call) {
    (void)first_call;
    if (mc_mq_busy()) {
        return GcodeStatus::Busy;
    }

    static const char kLetters[4] = { 'X', 'Y', 'Z', 'E' };
    bool has_any = false;
    for (uint8_t i = 0U; i < 4U; ++i) {
        if (mc_gcode_has(cmd, kLetters[i])) {
            has_any = true;
            break;
        }
    }

    if (!has_any) {
        mc_motor_disable_all();
    } else {
        for (uint8_t i = 0U; i < 4U; ++i) {
            if (mc_gcode_has(cmd, kLetters[i])) {
                (void)mc_motor_enable(static_cast<StepAxis>(i), false);
            }
        }
    }
    return GcodeStatus::Done;
}

}  // namespace mc
