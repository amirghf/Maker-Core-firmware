// M205: تنظیمات پیشرفته حرکت (J = junction deviation به mm، S = حداقل سرعت mm/s).
#include "gcode_handlers.h"
#include "motion_limits.h"
#include "planner.h"
#include "host_reply.h"
#include "text_format.h"
#include <math.h>

namespace mc {

GcodeStatus mc_gcode_m205(const GcodeCommand& cmd, bool first_call) {
    (void)first_call;
    const bool has_j = mc_gcode_has(cmd, 'J');
    const bool has_s = mc_gcode_has(cmd, 'S');

    if (!has_j && !has_s) {
        char buf[96];
        uint16_t n = 0U;
        const uint16_t cap = static_cast<uint16_t>(sizeof(buf));
        n = mc_fmt_text(buf, cap, n, "M205 S");
        n = mc_fmt_float(buf, cap, n, mc_mlim_minfeed_get(), 2U);
        n = mc_fmt_text(buf, cap, n, " J");
        n = mc_fmt_float(buf, cap, n, static_cast<float>(mc_plan_jd_get()) / 1000.0f, 3U);
        mc_reply_line(buf);
        return GcodeStatus::Done;
    }

    uint32_t j_um = 0U;
    float    s_val = 0.0f;

    if (has_j) {
        if (!mc_gcode_has_value(cmd, 'J')) { mc_mlim_note_refused(); return GcodeStatus::Error; }
        const float j_val = mc_gcode_value(cmd, 'J', 0.0f);
        if (isnan(j_val) || (j_val < 0.001f) || (j_val > 5.0f)) { mc_mlim_note_refused(); return GcodeStatus::Error; }
        j_um = static_cast<uint32_t>(lroundf(j_val * 1000.0f));
        if ((j_um < kPlanMinJunctionUm) || (j_um > kPlanMaxJunctionUm)) { mc_mlim_note_refused(); return GcodeStatus::Error; }
    }

    if (has_s) {
        if (!mc_gcode_has_value(cmd, 'S')) { mc_mlim_note_refused(); return GcodeStatus::Error; }
        s_val = mc_gcode_value(cmd, 'S', 0.0f);
        if (isnan(s_val) || (s_val < kMlimMinMinFeedMmS) || (s_val > kMlimMaxMinFeedMmS)) { mc_mlim_note_refused(); return GcodeStatus::Error; }
    }

    if (has_j) { (void)mc_plan_jd_set(j_um); }
    if (has_s) { (void)mc_mlim_minfeed_set(s_val); }

    return GcodeStatus::Done;
}

}  // namespace mc
