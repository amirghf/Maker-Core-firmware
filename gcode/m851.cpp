// M851: گزارش و تنظیم آفست پراب (بند ۶.۶).
// M851 (بدون پارامتر): گزارش آفست‌ها.
// M851 [X<mm>] [Y<mm>] [Z<mm>]: تنظیم آفست‌ها.
#include "gcode_handlers.h"
#include "probe.h"
#include "host_reply.h"
#include "text_format.h"
#include <math.h>

namespace mc {

GcodeStatus mc_gcode_m851(const GcodeCommand& cmd, bool first_call) {
    (void)first_call;

    const bool has_x = mc_gcode_has(cmd, 'X');
    const bool has_y = mc_gcode_has(cmd, 'Y');
    const bool has_z = mc_gcode_has(cmd, 'Z');

    if (!has_x && !has_y && !has_z) {
        char buf[96];
        uint16_t n = 0U;
        const uint16_t cap = static_cast<uint16_t>(sizeof(buf));
        n = mc_fmt_text(buf, cap, n, "Probe Offset X:");
        n = mc_fmt_float(buf, cap, n, mc_probe_offset_get(0U), 2U);
        n = mc_fmt_text(buf, cap, n, " Y:");
        n = mc_fmt_float(buf, cap, n, mc_probe_offset_get(1U), 2U);
        n = mc_fmt_text(buf, cap, n, " Z:");
        n = mc_fmt_float(buf, cap, n, mc_probe_offset_get(2U), 2U);
        mc_reply_line(buf);
        return GcodeStatus::Done;
    }

    // اول همه مقدارها بررسی می‌شوند
    float x_val = 0.0f;
    float y_val = 0.0f;
    float z_val = 0.0f;

    if (has_x) {
        if (!mc_gcode_has_value(cmd, 'X')) {
            mc_probe_note_refused();
            return GcodeStatus::Error;
        }
        x_val = mc_gcode_value(cmd, 'X', 0.0f);
        if (!mc_probe_offset_valid(0U, x_val)) {
            mc_probe_note_refused();
            return GcodeStatus::Error;
        }
    }

    if (has_y) {
        if (!mc_gcode_has_value(cmd, 'Y')) {
            mc_probe_note_refused();
            return GcodeStatus::Error;
        }
        y_val = mc_gcode_value(cmd, 'Y', 0.0f);
        if (!mc_probe_offset_valid(1U, y_val)) {
            mc_probe_note_refused();
            return GcodeStatus::Error;
        }
    }

    if (has_z) {
        if (!mc_gcode_has_value(cmd, 'Z')) {
            mc_probe_note_refused();
            return GcodeStatus::Error;
        }
        z_val = mc_gcode_value(cmd, 'Z', 0.0f);
        if (!mc_probe_offset_valid(2U, z_val)) {
            mc_probe_note_refused();
            return GcodeStatus::Error;
        }
    }

    // همه معتبر بودند، اعمال کن
    if (has_x) { (void)mc_probe_offset_set(0U, x_val); }
    if (has_y) { (void)mc_probe_offset_set(1U, y_val); }
    if (has_z) { (void)mc_probe_offset_set(2U, z_val); }

    return GcodeStatus::Done;
}

}  // namespace mc
