// M301: ثبت دستی ضرایب PID هات‌اند (بند ۱۱.۱). M301 [P<val>] [I<val>] [D<val>]
// هر ضریب باید عددی بین 0 و MC_M301_GAIN_MAX (1000.0) باشد.
// هر پارامتری که داده نشود مقدار فعلی بدون تغییر می‌ماند.
// بدون پارامتر: چاپ Kp:xx.xx Ki:xx.xx Kd:xx.xx
#include "gcode_handlers.h"
#include "heater_ex.h"
#include "host_reply.h"
#include "text_format.h"
#include "../config/mc_config.h"
#include <cmath>

extern "C" {
volatile uint32_t mc_dbg_500_m301_cnt = 0U; // خواندنی: تعداد دفعاتی که M301 با موفقیت ضرایب را اعمال کرد (بند ۱۱.۱)
}

namespace mc {

GcodeStatus mc_gcode_m301(const GcodeCommand& cmd, bool first_call) {
    (void)first_call;

    uint32_t cur_kp_x100 = 0U;
    uint32_t cur_ki_x100 = 0U;
    uint32_t cur_kd_x100 = 0U;
    mc_heater_ex_get_gains_x100(cur_kp_x100, cur_ki_x100, cur_kd_x100);

    const float cur_p = static_cast<float>(cur_kp_x100) / 100.0f;
    const float cur_i = static_cast<float>(cur_ki_x100) / 100.0f;
    const float cur_d = static_cast<float>(cur_kd_x100) / 100.0f;

    const bool has_p = mc_gcode_has(cmd, 'P');
    const bool has_i = mc_gcode_has(cmd, 'I');
    const bool has_d = mc_gcode_has(cmd, 'D');

    if (!has_p && !has_i && !has_d) {
        char buf[96];
        uint16_t n = 0U;
        const uint16_t cap = static_cast<uint16_t>(sizeof(buf));
        n = mc_fmt_text(buf, cap, n, "Kp:");
        n = mc_fmt_float(buf, cap, n, cur_p, 2U);
        n = mc_fmt_text(buf, cap, n, " Ki:");
        n = mc_fmt_float(buf, cap, n, cur_i, 2U);
        n = mc_fmt_text(buf, cap, n, " Kd:");
        n = mc_fmt_float(buf, cap, n, cur_d, 2U);
        mc_reply_line(buf);
        return GcodeStatus::Done;
    }

    float p_val = cur_p;
    float i_val = cur_i;
    float d_val = cur_d;

    if (has_p) {
        if (!mc_gcode_has_value(cmd, 'P')) {
            mc_reply_error("M301: missing P value");
            return GcodeStatus::Error;
        }
        p_val = mc_gcode_value(cmd, 'P', -1.0f);
        if (std::isnan(p_val) || (p_val < 0.0f) || (p_val > MC_M301_GAIN_MAX)) {
            mc_reply_error("M301: P gain out of range (0-1000)");
            return GcodeStatus::Error;
        }
    }

    if (has_i) {
        if (!mc_gcode_has_value(cmd, 'I')) {
            mc_reply_error("M301: missing I value");
            return GcodeStatus::Error;
        }
        i_val = mc_gcode_value(cmd, 'I', -1.0f);
        if (std::isnan(i_val) || (i_val < 0.0f) || (i_val > MC_M301_GAIN_MAX)) {
            mc_reply_error("M301: I gain out of range (0-1000)");
            return GcodeStatus::Error;
        }
    }

    if (has_d) {
        if (!mc_gcode_has_value(cmd, 'D')) {
            mc_reply_error("M301: missing D value");
            return GcodeStatus::Error;
        }
        d_val = mc_gcode_value(cmd, 'D', -1.0f);
        if (std::isnan(d_val) || (d_val < 0.0f) || (d_val > MC_M301_GAIN_MAX)) {
            mc_reply_error("M301: D gain out of range (0-1000)");
            return GcodeStatus::Error;
        }
    }

    const uint32_t new_kp_x100 = static_cast<uint32_t>(std::lround(p_val * 100.0f));
    const uint32_t new_ki_x100 = static_cast<uint32_t>(std::lround(i_val * 100.0f));
    const uint32_t new_kd_x100 = static_cast<uint32_t>(std::lround(d_val * 100.0f));

    mc_heater_ex_set_gains_x100(new_kp_x100, new_ki_x100, new_kd_x100);
    mc_dbg_500_m301_cnt = mc_dbg_500_m301_cnt + 1U;

    char buf[96];
    uint16_t n = 0U;
    const uint16_t cap = static_cast<uint16_t>(sizeof(buf));
    n = mc_fmt_text(buf, cap, n, "Kp:");
    n = mc_fmt_float(buf, cap, n, p_val, 2U);
    n = mc_fmt_text(buf, cap, n, " Ki:");
    n = mc_fmt_float(buf, cap, n, i_val, 2U);
    n = mc_fmt_text(buf, cap, n, " Kd:");
    n = mc_fmt_float(buf, cap, n, d_val, 2U);
    mc_reply_line(buf);

    return GcodeStatus::Done;
}

}  // namespace mc
