// M115: اطلاعات فرمویر و امکانات. بعداً با اضافه شدن امکانات، مقدار Cap ها ۱ می‌شود.
#include "gcode_handlers.h"
#include "host_reply.h"
#include "text_format.h"
#include "hal_board.h"
#include "../config/mc_version.h"    // پوشه config در مسیر include نیست؛ مسیر نسبی از همین فایل

namespace mc {
namespace {

// فهرست امکانات (۰ = فعلاً ندارد)
const char* const kCaps[] = {
    "Cap:SERIAL_XON_XOFF:0",
    "Cap:EEPROM:0",
    "Cap:AUTOREPORT_TEMP:1",
    "Cap:AUTOREPORT_POS:0",
    "Cap:SDCARD:0",
    "Cap:EMERGENCY_PARSER:0",
    "Cap:THERMAL_PROTECTION:1",
};

}  // namespace

GcodeStatus mc_gcode_m115(const GcodeCommand& cmd, bool first_call) {
    (void)cmd;
    (void)first_call;
    char buf[128];
    uint16_t n = 0U;
    const uint16_t cap = static_cast<uint16_t>(sizeof(buf));
    n = mc_fmt_text(buf, cap, n, "FIRMWARE_NAME:" MC_FIRMWARE_NAME " " MC_VERSION_STRING " PROTOCOL_VERSION:1.0 MACHINE_TYPE:");
    n = mc_fmt_text(buf, cap, n, mc_board_name());
    n = mc_fmt_text(buf, cap, n, " EXTRUDER_COUNT:1");
    mc_reply_line(buf);
    for (uint8_t i = 0U; i < static_cast<uint8_t>(sizeof(kCaps) / sizeof(kCaps[0])); ++i) {
        mc_reply_line(kCaps[i]);
    }
    return GcodeStatus::Done;
}

}  // namespace mc
