// M300: پخش بوق غیرمسدودکننده با بیزر (بند ۱۱.۳). M300 [S<فرکانس>] [P<میلی‌ثانیه>]
// S نادیده گرفته می‌شود (بیزر Active است). P پیش‌فرض MC_M300_DEFAULT_MS (200ms)، حداکثر MC_M300_MAX_MS (1000ms).
// پاسخ: ok
#include "gcode_handlers.h"
#include "hal_buzzer.h"
#include "../config/mc_config.h"

extern "C" {
volatile uint32_t mc_dbg_504_m300_cnt = 0U; // خواندنی: تعداد M300 پذیرفته‌شده (بند ۱۱.۳)
}

namespace mc {

GcodeStatus mc_gcode_m300(const GcodeCommand& cmd, bool first_call) {
    (void)first_call;

    int32_t p_ms = mc_gcode_int(cmd, 'P', static_cast<int32_t>(MC_M300_DEFAULT_MS));
    if (p_ms < 0) { p_ms = 0; }
    if (p_ms > static_cast<int32_t>(MC_M300_MAX_MS)) {
        p_ms = static_cast<int32_t>(MC_M300_MAX_MS);
    }

    if (p_ms > 0) {
        mc_buzzer_click(static_cast<uint32_t>(p_ms));
    }

    mc_dbg_504_m300_cnt = mc_dbg_504_m300_cnt + 1U;
    return GcodeStatus::Done;
}

}  // namespace mc
