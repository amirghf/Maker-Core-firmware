// M24: شروع یا ادامه چاپ (بند ۱۱.۳). فعلاً چون منبع چاپ فایل پیاده‌سازی نشده Error برمی‌گرداند.
#include "gcode_handlers.h"
#include "log.h"

namespace mc {

GcodeStatus mc_gcode_m24(const GcodeCommand& cmd, bool first_call) {
    (void)cmd;
    (void)first_call;
    mc_log(LogLevel::Warn, "M24: no print source");
    return GcodeStatus::Error;
}

}  // namespace mc
