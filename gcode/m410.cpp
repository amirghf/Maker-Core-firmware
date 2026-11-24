// M410: توقف سریع حرکت، پاک‌کردن صف‌ها، خاموش‌کردن هیترها و فن و غیرفعال‌کردن موتورها (بند ۸.۱۶.۵ و ۹.۱).
#include "gcode_handlers.h"
#include "quick_stop.h"

namespace mc {

GcodeStatus mc_gcode_m410(const GcodeCommand& cmd, bool first_call) {
    (void)cmd;
    (void)first_call;
    mc_quick_stop();
    return GcodeStatus::Done;
}

}  // namespace mc
