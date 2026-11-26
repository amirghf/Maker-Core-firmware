// M220: تنظیم و گزارش ضریب سرعت (بند ۱۱.۳). M220 [S<درصد>]
// بازه مجاز S از 10 تا 500 درصد. پیش‌فرض 100.
// بدون S: چاپ FR:100%
#include "gcode_handlers.h"
#include "motion_cmd.h"
#include "host_reply.h"
#include "text_format.h"
#include "../config/mc_config.h"
#include <cmath>

namespace mc {

GcodeStatus mc_gcode_m220(const GcodeCommand& cmd, bool first_call) {
    (void)first_call;

    if (!mc_gcode_has(cmd, 'S')) {
        char buf[48];
        uint16_t n = 0U;
        const uint16_t cap = static_cast<uint16_t>(sizeof(buf));
        n = mc_fmt_text(buf, cap, n, "FR:");
        n = mc_fmt_uint(buf, cap, n, mc_motion_speed_factor_get());
        n = mc_fmt_text(buf, cap, n, "%");
        mc_reply_line(buf);
        return GcodeStatus::Done;
    }

    if (!mc_gcode_has_value(cmd, 'S')) {
        mc_reply_error("M220: missing S value");
        return GcodeStatus::Error;
    }

    const float s_val = mc_gcode_value(cmd, 'S', -1.0f);
    if (std::isnan(s_val) || (s_val < static_cast<float>(MC_SPEED_FACTOR_MIN_PCT)) ||
        (s_val > static_cast<float>(MC_SPEED_FACTOR_MAX_PCT))) {
        mc_reply_error("M220: speed factor out of range (10-500)");
        return GcodeStatus::Error;
    }

    const uint32_t pct = static_cast<uint32_t>(std::lround(s_val));
    if (!mc_motion_speed_factor_set(pct)) {
        mc_reply_error("M220: speed factor set failed");
        return GcodeStatus::Error;
    }

    return GcodeStatus::Done;
}

}  // namespace mc
