// M302: اکستروژن سرد (بند ۵.۹).
//   M302 S170  ← حداقل دمای اکستروژن 170 درجه (بازه ۰ تا ۲۷۵)
//   M302 P1    ← اکستروژن سرد مجاز    M302 P0 ← ممنوع
//   M302       ← فقط وضعیت را چاپ می‌کند
// اگر یکی از مقدارها نامعتبر باشد (بدون عدد یا بیرون از بازه)، هیچ‌کدام اعمال نمی‌شود و Error برمی‌گردد.
#include "gcode_handlers.h"
#include "extruder.h"
#include "host_reply.h"
#include "text_format.h"

namespace mc {

GcodeStatus mc_gcode_m302(const GcodeCommand& cmd, bool first_call) {
    (void)first_call;
    const bool has_s = mc_gcode_has(cmd, 'S');
    const bool has_p = mc_gcode_has(cmd, 'P');
    int32_t s_x10 = 0;
    bool    p_on  = false;

    if (has_s) {
        if (!mc_gcode_has_value(cmd, 'S')) { return GcodeStatus::Error; }
        const float s = mc_gcode_value(cmd, 'S', 0.0f);
        if (!((s >= 0.0f) && (s <= static_cast<float>(kExMaxMinTempDeg)))) { return GcodeStatus::Error; }   // NaN هم رد می‌شود
        s_x10 = static_cast<int32_t>((s * 10.0f) + 0.5f);
    }
    if (has_p) {
        if (!mc_gcode_has_value(cmd, 'P')) { return GcodeStatus::Error; }
        const float p = mc_gcode_value(cmd, 'P', -1.0f);
        if (p == 1.0f) {
            p_on = true;
        } else if (p == 0.0f) {
            p_on = false;
        } else {
            return GcodeStatus::Error;
        }
    }

    if (has_s) { (void)mc_extruder_min_temp_x10_set(s_x10); }
    if (has_p) { mc_extruder_cold_ok_set(p_on); }

    if (!has_s && !has_p) {
        char buf[96];
        uint16_t n = 0U;
        const uint16_t cap = static_cast<uint16_t>(sizeof(buf));
        n = mc_fmt_text(buf, cap, n, "echo:Cold extrudes are ");
        n = mc_fmt_text(buf, cap, n, mc_extruder_cold_ok_get() ? "enabled" : "disabled");
        n = mc_fmt_text(buf, cap, n, " (min temp ");
        n = mc_fmt_uint(buf, cap, n, static_cast<uint32_t>((mc_extruder_min_temp_x10_get() + 5) / 10));
        n = mc_fmt_text(buf, cap, n, "C)");
        mc_reply_line(buf);
    }
    return GcodeStatus::Done;
}

}  // namespace mc
