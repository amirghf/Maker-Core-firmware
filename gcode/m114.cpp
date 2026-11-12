// M114: موقعیت فعلی محورها را نشان می‌دهد. مثل: X:0.00 Y:0.00 Z:0.00 E:0.00
#include "gcode_handlers.h"
#include "host_reply.h"
#include "machine_state.h"
#include "text_format.h"

namespace mc {

GcodeStatus mc_gcode_m114(const GcodeCommand& cmd, bool first_call) {
    (void)cmd;
    (void)first_call;
    char buf[96];
    uint16_t n = 0U;
    const uint16_t cap = static_cast<uint16_t>(sizeof(buf));
    n = mc_fmt_text(buf, cap, n, "X:");
    n = mc_fmt_float(buf, cap, n, mc_position_get(kAxisX), 2U);
    n = mc_fmt_text(buf, cap, n, " Y:");
    n = mc_fmt_float(buf, cap, n, mc_position_get(kAxisY), 2U);
    n = mc_fmt_text(buf, cap, n, " Z:");
    n = mc_fmt_float(buf, cap, n, mc_position_get(kAxisZ), 2U);
    n = mc_fmt_text(buf, cap, n, " E:");
    n = mc_fmt_float(buf, cap, n, mc_position_get(kAxisE), 2U);
    mc_reply_line(buf);
    return GcodeStatus::Done;
}

}  // namespace mc
