// M503: گزارش تنظیمات فعلی از RAM (بند ۷.۳).
#include "gcode_handlers.h"
#include "settings_data.h"
#include "motion_modes.h"
#include "motion_limits.h"
#include "planner.h"
#include "soft_limits.h"
#include "probe.h"
#include "extruder.h"
#include "heater_ex.h"
#include "host_reply.h"
#include "text_format.h"

namespace mc {

GcodeStatus mc_gcode_m503(const GcodeCommand& cmd, bool first_call) {
    (void)cmd;
    (void)first_call;

    char buf[96];
    uint16_t n = 0U;
    const uint16_t cap = static_cast<uint16_t>(sizeof(buf));

    // ۱. M92
    n = 0U;
    n = mc_fmt_text(buf, cap, n, "M92 X");
    n = mc_fmt_float(buf, cap, n, mc_mode_spmm_get(0U), 2U);
    n = mc_fmt_text(buf, cap, n, " Y");
    n = mc_fmt_float(buf, cap, n, mc_mode_spmm_get(1U), 2U);
    n = mc_fmt_text(buf, cap, n, " Z");
    n = mc_fmt_float(buf, cap, n, mc_mode_spmm_get(2U), 2U);
    n = mc_fmt_text(buf, cap, n, " E");
    n = mc_fmt_float(buf, cap, n, mc_mode_spmm_get(3U), 2U);
    mc_reply_line(buf);

    // ۲. M203
    n = 0U;
    n = mc_fmt_text(buf, cap, n, "M203 X");
    n = mc_fmt_float(buf, cap, n, mc_mlim_feed_get(0U), 2U);
    n = mc_fmt_text(buf, cap, n, " Y");
    n = mc_fmt_float(buf, cap, n, mc_mlim_feed_get(1U), 2U);
    n = mc_fmt_text(buf, cap, n, " Z");
    n = mc_fmt_float(buf, cap, n, mc_mlim_feed_get(2U), 2U);
    n = mc_fmt_text(buf, cap, n, " E");
    n = mc_fmt_float(buf, cap, n, mc_mlim_feed_get(3U), 2U);
    mc_reply_line(buf);

    // ۳. M204
    n = 0U;
    n = mc_fmt_text(buf, cap, n, "M204 P");
    n = mc_fmt_float(buf, cap, n, mc_mlim_acc_get(0U), 2U);
    n = mc_fmt_text(buf, cap, n, " R");
    n = mc_fmt_float(buf, cap, n, mc_mlim_acc_get(1U), 2U);
    n = mc_fmt_text(buf, cap, n, " T");
    n = mc_fmt_float(buf, cap, n, mc_mlim_acc_get(2U), 2U);
    mc_reply_line(buf);

    // ۴. M205
    n = 0U;
    n = mc_fmt_text(buf, cap, n, "M205 S");
    n = mc_fmt_float(buf, cap, n, mc_mlim_minfeed_get(), 2U);
    n = mc_fmt_text(buf, cap, n, " J");
    n = mc_fmt_float(buf, cap, n, static_cast<float>(mc_plan_jd_get()) / 1000.0f, 3U);
    mc_reply_line(buf);

    // ۵. M211
    n = 0U;
    n = mc_fmt_text(buf, cap, n, mc_softlimit_enabled_get() ? "M211 S1" : "M211 S0");
    mc_reply_line(buf);

    // ۶. M851
    n = 0U;
    n = mc_fmt_text(buf, cap, n, "Probe Offset X:");
    n = mc_fmt_float(buf, cap, n, mc_probe_offset_get(0U), 2U);
    n = mc_fmt_text(buf, cap, n, " Y:");
    n = mc_fmt_float(buf, cap, n, mc_probe_offset_get(1U), 2U);
    n = mc_fmt_text(buf, cap, n, " Z:");
    n = mc_fmt_float(buf, cap, n, mc_probe_offset_get(2U), 2U);
    mc_reply_line(buf);

    // ۷. M302
    n = 0U;
    n = mc_fmt_text(buf, cap, n, "M302 S");
    n = mc_fmt_float(buf, cap, n, static_cast<float>(mc_extruder_min_temp_x10_get()) / 10.0f, 1U);
    n = mc_fmt_text(buf, cap, n, mc_extruder_cold_ok_get() ? " P1" : " P0");
    mc_reply_line(buf);

    // ۸. PID
    uint32_t kp = 0U, ki = 0U, kd = 0U;
    mc_heater_ex_get_gains_x100(kp, ki, kd);
    n = 0U;
    n = mc_fmt_text(buf, cap, n, "Hotend PID P:");
    n = mc_fmt_float(buf, cap, n, static_cast<float>(kp) / 100.0f, 2U);
    n = mc_fmt_text(buf, cap, n, " I:");
    n = mc_fmt_float(buf, cap, n, static_cast<float>(ki) / 100.0f, 2U);
    n = mc_fmt_text(buf, cap, n, " D:");
    n = mc_fmt_float(buf, cap, n, static_cast<float>(kd) / 100.0f, 2U);
    mc_reply_line(buf);

    mc_cfg_note(503U, 1U, 0U, 0U);
    return GcodeStatus::Done;
}

}  // namespace mc
