// M280: فرمان سروو BLTouch (بند ۶.۵). شکل: M280 P0 S<زاویه>
// فقط P0 و زاویه‌های ۱۰ (بیرون)، ۹۰ (جمع)، ۱۲۰ (تست خود پراب)، ۱۶۰ (آزاد کردن آلارم) مجازند.
// هر چیز دیگر رد می‌شود (زاویه‌های تنظیم دائمی BLTouch عمداً مجاز نیستند).
#include "gcode_handlers.h"
#include "bltouch.h"
#include "machine_state.h"
#include <math.h>

namespace mc {

GcodeStatus mc_gcode_m280(const GcodeCommand& cmd, bool first_call) {
    static bool        started = false;
    static BltouchCmd  pending = BltouchCmd::Stow;

    if (first_call) {
        started = false;
        if (!mc_gcode_has_value(cmd, 'P') || !mc_gcode_has_value(cmd, 'S')) {
            mc_bltouch_note_refused();
            return GcodeStatus::Error;
        }
        const float p = mc_gcode_value(cmd, 'P', -1.0f);
        const float s = mc_gcode_value(cmd, 'S', -1.0f);
        if (isnan(p) || isnan(s) || (p != 0.0f) || (s < 0.0f) || (s > 255.0f)) {
            mc_bltouch_note_refused();
            return GcodeStatus::Error;
        }
        const uint32_t angle = static_cast<uint32_t>(s);
        if (static_cast<float>(angle) != s) {
            mc_bltouch_note_refused();
            return GcodeStatus::Error;
        }
        if (!mc_bltouch_cmd_from_angle(angle, pending)) {
            mc_bltouch_note_refused();
            return GcodeStatus::Error;
        }
    }

    if (!started) {
        if (mc_motion_busy()) { return GcodeStatus::Busy; }
        if (!mc_bltouch_start(pending)) { return GcodeStatus::Error; }
        started = true;
    }
    if (mc_bltouch_busy()) { return GcodeStatus::Busy; }
    return GcodeStatus::Done;
}

}  // namespace mc
