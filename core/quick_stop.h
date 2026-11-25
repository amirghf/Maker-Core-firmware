#pragma once
// STOP (بند ۸.۱۶.۵ و ۹.۲ و ۱۰.۱۴): توقف سریع حرکت، پاک‌کردن صف‌ها، خاموش‌کردن هیترها و فن و غیرفعال‌کردن موتورها.
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// متغیر دیباگ توقف سریع (بند ۱۰.۱۴)
extern volatile uint32_t mc_dbg_489_qstop_sync_cnt; // خواندنی: تعداد دفعاتی که بعد از توقف سریع موقعیت منطقی از گام‌های واقعی دوباره ساخته شد

#ifdef __cplusplus
}

namespace mc {

// STOP: حرکت‌ها و صف قطع می‌شود، موقعیت منطقی بازسازی می‌شود، هیتر اکسترودر و بستر و فن خاموش می‌شوند و موتورها غیرفعال می‌شوند.
void mc_quick_stop();

}  // namespace mc
#endif
