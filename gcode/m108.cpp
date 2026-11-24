// M108 (بند ۹.۱): ادامه/لغو انتظار در دستورهای منتظر (تعویض فیلامنت).
#include "gcode_handlers.h"
#include "../core/filament_change.h"

namespace mc {

GcodeStatus mc_gcode_m108(const GcodeCommand& cmd, bool first_call) {
    (void)cmd;
    (void)first_call;
    mc_filchg_resume_request(3U);
    return GcodeStatus::Done;
}

}  // namespace mc
