// M92: گام بر میلی‌متر هر محور. مثل M92 X80 Y80 Z400 E93
// بدون پارامتر: مقدارهای فعلی را چاپ می‌کند (M92 X.. Y.. Z.. E..).
// مقدار مجاز: ۱ تا ۱۰۰۰۰. اگر یکی از مقدارها نامعتبر باشد (بدون عدد یا بیرون از بازه)، هیچ‌کدام اعمال نمی‌شود و Error برمی‌گردد.
// تا وقتی بلوک حرکت در صف یا در اجراست صبر می‌کند (Busy)، چون برنامه‌ریز از این عدد برای هندسه بلوک‌ها استفاده می‌کند.
#include "gcode_handlers.h"
#include "host_reply.h"
#include "motion_modes.h"
#include "motion_queue.h"
#include "text_format.h"

namespace mc {

GcodeStatus mc_gcode_m92(const GcodeCommand& cmd, bool first_call) {
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
            mc_mode_note_refused();
            return GcodeStatus::Error;
        }
        val[i] = mc_gcode_value(cmd, kLetters[i], 0.0f);
        if (!((val[i] >= kModeMinStepsPerMm) && (val[i] <= kModeMaxStepsPerMm))) {   // NaN هم رد می‌شود
            mc_mode_note_refused();
            return GcodeStatus::Error;
        }
    }

    if (!any) {
        char buf[96];
        uint16_t n = 0U;
        const uint16_t cap = static_cast<uint16_t>(sizeof(buf));
        n = mc_fmt_text(buf, cap, n, "M92 X");
        n = mc_fmt_float(buf, cap, n, mc_mode_spmm_get(0U), 2U);
        n = mc_fmt_text(buf, cap, n, " Y");
        n = mc_fmt_float(buf, cap, n, mc_mode_spmm_get(1U), 2U);
        n = mc_fmt_text(buf, cap, n, " Z");
        n = mc_fmt_float(buf, cap, n, mc_mode_spmm_get(2U), 2U);
        n = mc_fmt_text(buf, cap, n, " E");
        n = mc_fmt_float(buf, cap, n, mc_mode_spmm_get(3U), 2U);
        mc_reply_line(buf);
        return GcodeStatus::Done;
    }

    if (mc_mq_busy()) { return GcodeStatus::Busy; }

    for (uint8_t i = 0U; i < 4U; ++i) {
        if (has[i]) { (void)mc_mode_spmm_set(i, val[i]); }
    }
    mc_mode_note_applied();
    return GcodeStatus::Done;
}

}  // namespace mc
