// M112: توقف اضطراری کامل (بند ۱۰.۱۴).
#include "gcode_handlers.h"
#include "error.h"

extern "C" {
volatile uint32_t mc_dbg_485_estop_cnt = 0U;  // خواندنی: تعداد M112 اجراشده (بند ۱۰.۱۴)
}

namespace mc {

GcodeStatus mc_gcode_m112(const GcodeCommand& cmd, bool first_call) {
    (void)cmd;
    (void)first_call;
    mc_dbg_485_estop_cnt = mc_dbg_485_estop_cnt + 1U;
    mc_kill(ErrorCode::EmergencyStop);
}

}  // namespace mc
