// M410: توقف سریع حرکت، پاک‌کردن صف‌ها، خاموش‌کردن هیترها و فن و غیرفعال‌کردن موتورها (بند ۸.۱۶.۵ و ۱۰.۱۴).
#include "gcode_handlers.h"
#include "quick_stop.h"

extern "C" {
volatile uint32_t mc_dbg_486_qstop_cnt      = 0U;  // خواندنی: تعداد M410 اجراشده (بند ۱۰.۱۴)
volatile uint32_t mc_dbg_487_qstop_busy_cnt = 0U;  // خواندنی: تعداد اجرا در حالت Busy (بند ۱۰.۱۴)
}

namespace mc {

GcodeStatus mc_gcode_m410(const GcodeCommand& cmd, bool first_call) {
    (void)cmd;
    (void)first_call;
    mc_dbg_486_qstop_cnt = mc_dbg_486_qstop_cnt + 1U;
    mc_quick_stop();
    return GcodeStatus::Done;
}

}  // namespace mc
