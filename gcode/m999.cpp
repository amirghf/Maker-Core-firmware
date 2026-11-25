// M999: ریست کامل سخت‌افزاری برد (بند ۸.۱۶.۵).
#include "gcode_handlers.h"
#include "hal_board.h"

namespace mc {

GcodeStatus mc_gcode_m999(const GcodeCommand& cmd, bool first_call) {
    (void)cmd;
    (void)first_call;
    mc_system_reset();
    return GcodeStatus::Done;
}

}  // namespace mc
