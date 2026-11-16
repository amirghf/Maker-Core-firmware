// M203: حداکثر سرعت هر محور (mm/s).
#include "gcode_handlers.h"
#include "motion_limits.h"
#include "host_reply.h"
#include "text_format.h"
#include <math.h>

namespace mc {

GcodeStatus mc_gcode_m203(const GcodeCommand& cmd, bool first_call) {
    (void)first_call;
    static const char kLetters[4] = { 'X', 'Y', 'Z', 'E' };
    bool  has[4];
    float val[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    bool  any = false;

    for (uint8_t i = 0U; i < 4U; ++i) {
        has[i] = mc_gcode_has(cmd, kLetters[i]);
        if (!has[i]) { continue; }
        any = true;
        if (!mc_gcode_has_value(cmd, kLetters[i])) {
            mc_mlim_note_refused();
            return GcodeStatus::Error;
        }
        val[i] = mc_gcode_value(cmd, kLetters[i], 0.0f);
        if (isnan(val[i]) || (val[i] < kMlimMinFeedMmS) || (val[i] > kMlimMaxFeedMmS)) {
            mc_mlim_note_refused();
            return GcodeStatus::Error;
        }
    }

    if (!any) {
        char buf[96];
        uint16_t n = 0U;
        const uint16_t cap = static_cast<uint16_t>(sizeof(buf));
        n = mc_fmt_text(buf, cap, n, "M203 X");
        n = mc_fmt_float(buf, cap, n, mc_mlim_feed_get(0U), 2U);
        n = mc_fmt_text(buf, cap, n, " Y");
        n = mc_fmt_float(buf, cap, n, mc_mlim_feed_get(1U), 2U);
        n = mc_fmt_text(buf, cap, n, " Z");
        n = mc_fmt_float(buf, cap, n, mc_mlim_feed_get(2U), 2U);
        n = mc_fmt_text(buf, cap, n, " E");
        n = mc_fmt_float(buf, cap, n, mc_mlim_feed_get(3U), 2U);
        mc_reply_line(buf);
        return GcodeStatus::Done;
    }

    for (uint8_t i = 0U; i < 4U; ++i) {
        if (has[i]) {
            (void)mc_mlim_feed_set(i, val[i]);
        }
    }
    return GcodeStatus::Done;
}

}  // namespace mc
