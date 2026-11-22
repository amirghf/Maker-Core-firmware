// M502: بازگشت تنظیمات در RAM به مقادیر پیش‌فرض (بند ۷.۳).
#include "gcode_handlers.h"
#include "settings_data.h"
#include "motion_queue.h"
#include "host_reply.h"

namespace mc {

GcodeStatus mc_gcode_m502(const GcodeCommand& cmd, bool first_call) {
    (void)cmd;
    (void)first_call;

    if (mc_mq_busy()) {
        return GcodeStatus::Busy;
    }

    mc_cfg_apply_defaults();
    mc_cfg_note(502U, 1U, 0U, 0U);
    mc_reply_line("Settings reset to defaults (not saved)");
    return GcodeStatus::Done;
}

}  // namespace mc
