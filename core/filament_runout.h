#pragma once
// واکنش خودکار به اتمام فیلامنت (دستور M600 خودکار - بند ۹.۴).
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// متغیرهای دیباگ واکنش به اتمام فیلامنت (بند ۹.۴)
extern volatile uint32_t mc_dbg_458_frun_last;     // خواندنی: آخرین تصمیم (0 هیچ، 1 M600 در صف، 2 رد - چاپ نیست، 3 رد - تعویض/بارگذاری فعال، 4 رد - killed، 5 صف پر بود و دوباره تلاش می‌شود)
extern volatile uint32_t mc_dbg_459_frun_trig_cnt; // خواندنی: تعداد M600هایی که با اتمام فیلامنت در صف گذاشته شدند
extern volatile uint32_t mc_dbg_460_frun_ign_cnt;  // خواندنی: تعداد رویدادهای ردشده
extern volatile uint32_t mc_dbg_463_frun_caused;   // خواندنی: 1 = M600 جاری یا منتظر شروع، توسط اتمام فیلامنت ایجاد شده

#ifdef __cplusplus
}

namespace mc {

// راه‌اندازی ماژول واکنش به اتمام فیلامنت
void mc_filrun_init();

// وظیفه زمان‌بند (هر ۱۰۰ میلی‌ثانیه): رویدادهای اتمام فیلامنت را پایش می‌کند
void mc_filrun_poll();

// آیا M600 جاری یا منتظر شروع، توسط اتمام فیلامنت ایجاد شده است؟
bool mc_filrun_caused();

// لغو پرچم‌ها (برای quick stop)
void mc_filrun_abort();

}  // namespace mc
#endif
