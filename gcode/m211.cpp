// M211: حدود نرم‌افزاری (بند ۶.۴).
// M211 (بدون S): گزارش وضعیت و حدود نرم‌افزاری.
// M211 S1: فعال‌کردن حدود نرم‌افزاری.
// M211 S0: غیرفعال‌کردن حدود نرم‌افزاری.
#include "gcode_handlers.h"
#include "soft_limits.h"
#include "host_reply.h"
#include "text_format.h"
#include <math.h>

// متغیر دیباگ؛ در CubeIDE با Live Expressions دیده می‌شود.
extern "C" {
volatile uint32_t mc_dbg_216_m211_runs = 0;  // خواندنی: تعداد اجراهای موفق دستور M211
}

namespace mc {

GcodeStatus mc_gcode_m211(const GcodeCommand& cmd, bool first_call) {
    (void)first_call;

    const bool has_s = mc_gcode_has(cmd, 'S');
    if (has_s) {
        if (!mc_gcode_has_value(cmd, 'S')) {
            mc_softlimit_note_refused();
            return GcodeStatus::Error;
        }
        const float s_val = mc_gcode_value(cmd, 'S', -1.0f);
        if (isnan(s_val) || !((s_val == 0.0f) || (s_val == 1.0f))) {
            mc_softlimit_note_refused();
            return GcodeStatus::Error;
        }
        mc_softlimit_enabled_set(s_val == 1.0f);
        mc_dbg_216_m211_runs = mc_dbg_216_m211_runs + 1U;
        return GcodeStatus::Done;
    }

    char buf[128];
    uint16_t n = 0U;
    const uint16_t cap = static_cast<uint16_t>(sizeof(buf));
    const bool enabled = mc_softlimit_enabled_get();

    n = mc_fmt_text(buf, cap, n, enabled ? "Soft endstops: ON  Min: X" : "Soft endstops: OFF  Min: X");
    n = mc_fmt_float(buf, cap, n, mc_softlimit_min_get(0U), 2U);
    n = mc_fmt_text(buf, cap, n, " Y");
    n = mc_fmt_float(buf, cap, n, mc_softlimit_min_get(1U), 2U);
    n = mc_fmt_text(buf, cap, n, " Z");
    n = mc_fmt_float(buf, cap, n, mc_softlimit_min_get(2U), 2U);
    n = mc_fmt_text(buf, cap, n, "  Max: X");
    n = mc_fmt_float(buf, cap, n, mc_softlimit_max_get(0U), 2U);
    n = mc_fmt_text(buf, cap, n, " Y");
    n = mc_fmt_float(buf, cap, n, mc_softlimit_max_get(1U), 2U);
    n = mc_fmt_text(buf, cap, n, " Z");
    n = mc_fmt_float(buf, cap, n, mc_softlimit_max_get(2U), 2U);

    mc_reply_line(buf);

    mc_dbg_216_m211_runs = mc_dbg_216_m211_runs + 1U;
    return GcodeStatus::Done;
}

}  // namespace mc
