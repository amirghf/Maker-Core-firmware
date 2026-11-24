// M25: توقف موقت چاپ (بند ۱۱.۵). فعلاً چون منبع چاپ فایل پیاده‌سازی نشده Error برمی‌گرداند.
#include "gcode_handlers.h"
#include "log.h"

namespace mc {

GcodeStatus mc_gcode_m25(const GcodeCommand& cmd, bool first_call) {
    (void)cmd;
    (void)first_call;
    mc_log(LogLevel::Warn, "M25: no print source");
    return GcodeStatus::Error;
}

}  // namespace mc
