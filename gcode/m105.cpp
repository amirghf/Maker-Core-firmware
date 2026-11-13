// M105: گزارش دما. جواب در همان خط ok است: "ok T:25.0 /0.0 B:25.0 /0.0 @:0 B@:0".
#include "gcode_handlers.h"
#include "temp_report.h"
#include "host_reply.h"

namespace mc {

GcodeStatus mc_gcode_m105(const GcodeCommand& cmd, bool first_call) {
    (void)cmd;
    (void)first_call;
    char buf[96];
    (void)mc_temp_report_build(buf, static_cast<uint16_t>(sizeof(buf)));
    mc_reply_ok_line(buf);
    return GcodeStatus::DoneReplied;   // ok را خودمان فرستادیم
}

}  // namespace mc
