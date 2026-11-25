// M119: گزارش وضعیت اندستاپ‌ها (بند ۶.۴).
// هیچ پارامتری ندارد. پارامتر اضافه نادیده گرفته می‌شود.
#include "gcode_handlers.h"
#include "endstop.h"
#include "host_reply.h"
#include "../core/filament_sensor.h"

// متغیر دیباگ؛ در CubeIDE با Live Expressions دیده می‌شود.
extern "C" {
volatile uint32_t mc_dbg_215_m119_runs = 0;  // خواندنی: تعداد اجراهای دستور M119
}

namespace mc {

GcodeStatus mc_gcode_m119(const GcodeCommand& cmd, bool first_call) {
    (void)cmd;
    (void)first_call;

    mc_dbg_215_m119_runs = mc_dbg_215_m119_runs + 1U;

    mc_reply_line("Reporting endstop status");
    mc_reply_line(mc_endstop_triggered(0U) ? "x_min: TRIGGERED" : "x_min: open");
    mc_reply_line(mc_endstop_triggered(1U) ? "y_min: TRIGGERED" : "y_min: open");
    mc_reply_line(mc_endstop_triggered(2U) ? "z_min: TRIGGERED" : "z_min: open");
    mc_reply_line(mc_filament_empty() ? "filament: TRIGGERED" : "filament: open");

    return GcodeStatus::Done;
}

}  // namespace mc
