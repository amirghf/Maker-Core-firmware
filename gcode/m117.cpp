// M117: نمایش پیام روی نمایشگر (بند ۱۱.۳). M117 <متن>
// حداکثر MC_M117_TEXT_MAX (40) کاراکتر؛ متن بلندتر بریده می‌شود.
// پاسخ: ok
#include "gcode_handlers.h"
#include "alert.h"
#include "../config/mc_config.h"

extern "C" {
volatile uint32_t mc_dbg_503_m117_cnt = 0U; // خواندنی: تعداد M117 پذیرفته‌شده (بند ۱۱.۳)
}

namespace mc {

GcodeStatus mc_gcode_m117(const GcodeCommand& cmd, bool first_call) {
    (void)first_call;

    const char* raw_text = mc_gcode_args(cmd);
    char buf[MC_M117_TEXT_MAX + 1U];
    uint16_t n = 0U;
    if (raw_text != nullptr) {
        while ((raw_text[n] != '\0') && (n < MC_M117_TEXT_MAX)) {
            buf[n] = raw_text[n];
            n++;
        }
    }
    buf[n] = '\0';

    if (n > 0U) {
        mc_alert_raise(AlertLevel::Info, buf, "M117 MESSAGE");
    } else {
        mc_alert_raise(AlertLevel::Info, "M117 MESSAGE", "");
    }

    mc_dbg_503_m117_cnt = mc_dbg_503_m117_cnt + 1U;
    return GcodeStatus::Done;
}

}  // namespace mc
