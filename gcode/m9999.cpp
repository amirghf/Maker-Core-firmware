// دستور تست M9999 برای بند ۴.۵ (موقت؛ دستورهای واقعی از بند ۴.۷ می‌آیند).
// فرمت: M9999 X<عدد> P<میلی‌ثانیه>
//   X فقط برای دیدن در متغیر دیباگ است. P مدت «مشغول بودن» دستور است (پیش‌فرض ۰، حداکثر ۱۰۰۰۰).
// این دستور نشان می‌دهد دستوری که کار طولانی دارد، بدون انتظار (Busy) اجرا می‌شود.
#include "gcode_handlers.h"
#include "hal_time.h"
#include "log.h"

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_m9999_runs = 0;    // تعداد دفعاتی که M9999 اجرا شد
volatile float    mc_dbg_m9999_x    = 0;    // مقدار X آخرین M9999 (اگر نبود: -۹۹۹)
}

namespace mc {

GcodeStatus mc_gcode_m9999(const GcodeCommand& cmd, bool first_call) {
    static uint32_t start_ms = 0U;
    static uint32_t wait_ms  = 0U;

    if (first_call) {
        start_ms = mc_millis();
        int32_t p = mc_gcode_int(cmd, 'P', 0);
        if (p < 0) { p = 0; }
        if (p > 10000) { p = 10000; }
        wait_ms = static_cast<uint32_t>(p);
        mc_dbg_m9999_x = mc_gcode_value(cmd, 'X', -999.0f);
        mc_log(LogLevel::Info, "M9999 start");
    }

    if (!mc_time_elapsed_ms(start_ms, wait_ms)) {
        return GcodeStatus::Busy;
    }

    mc_dbg_m9999_runs = mc_dbg_m9999_runs + 1U;
    mc_log(LogLevel::Info, "M9999 done");
    return GcodeStatus::Done;
}

}  // namespace mc
