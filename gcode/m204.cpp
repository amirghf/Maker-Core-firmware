// M204: شتاب (mm/s²).
#include "gcode_handlers.h"
#include "motion_limits.h"
#include "host_reply.h"
#include "text_format.h"
#include <math.h>

namespace mc {

GcodeStatus mc_gcode_m204(const GcodeCommand& cmd, bool first_call) {
    (void)first_call;
    const bool has_p = mc_gcode_has(cmd, 'P');
    const bool has_r = mc_gcode_has(cmd, 'R');
    const bool has_t = mc_gcode_has(cmd, 'T');
    const bool has_s = mc_gcode_has(cmd, 'S');

    if (!has_p && !has_r && !has_t && !has_s) {
        char buf[96];
        uint16_t n = 0U;
        const uint16_t cap = static_cast<uint16_t>(sizeof(buf));
        n = mc_fmt_text(buf, cap, n, "M204 P");
        n = mc_fmt_float(buf, cap, n, mc_mlim_acc_get(0U), 2U);
        n = mc_fmt_text(buf, cap, n, " R");
        n = mc_fmt_float(buf, cap, n, mc_mlim_acc_get(1U), 2U);
        n = mc_fmt_text(buf, cap, n, " T");
        n = mc_fmt_float(buf, cap, n, mc_mlim_acc_get(2U), 2U);
        mc_reply_line(buf);
        return GcodeStatus::Done;
    }

    float p_val = 0.0f, r_val = 0.0f, t_val = 0.0f, s_val = 0.0f;

    if (has_p) {
        if (!mc_gcode_has_value(cmd, 'P')) { mc_mlim_note_refused(); return GcodeStatus::Error; }
        p_val = mc_gcode_value(cmd, 'P', 0.0f);
        if (isnan(p_val) || (p_val < kMlimMinAccMmS2) || (p_val > kMlimMaxAccMmS2)) { mc_mlim_note_refused(); return GcodeStatus::Error; }
    }
    if (has_r) {
        if (!mc_gcode_has_value(cmd, 'R')) { mc_mlim_note_refused(); return GcodeStatus::Error; }
        r_val = mc_gcode_value(cmd, 'R', 0.0f);
        if (isnan(r_val) || (r_val < kMlimMinAccMmS2) || (r_val > kMlimMaxAccMmS2)) { mc_mlim_note_refused(); return GcodeStatus::Error; }
    }
    if (has_t) {
        if (!mc_gcode_has_value(cmd, 'T')) { mc_mlim_note_refused(); return GcodeStatus::Error; }
        t_val = mc_gcode_value(cmd, 'T', 0.0f);
        if (isnan(t_val) || (t_val < kMlimMinAccMmS2) || (t_val > kMlimMaxAccMmS2)) { mc_mlim_note_refused(); return GcodeStatus::Error; }
    }
    if (has_s) {
        if (!mc_gcode_has_value(cmd, 'S')) { mc_mlim_note_refused(); return GcodeStatus::Error; }
        s_val = mc_gcode_value(cmd, 'S', 0.0f);
        if (isnan(s_val) || (s_val < kMlimMinAccMmS2) || (s_val > kMlimMaxAccMmS2)) { mc_mlim_note_refused(); return GcodeStatus::Error; }
    }

    if (has_p) { (void)mc_mlim_acc_set(0U, p_val); }
    else if (has_s) { (void)mc_mlim_acc_set(0U, s_val); }

    if (has_r) { (void)mc_mlim_acc_set(1U, r_val); }

    if (has_t) { (void)mc_mlim_acc_set(2U, t_val); }
    else if (has_s) { (void)mc_mlim_acc_set(2U, s_val); }

    return GcodeStatus::Done;
}

}  // namespace mc
