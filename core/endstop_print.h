#pragma once
// بررسی اندستاپ حین چاپ (بند ۱۰.۸): اگر دستگاه در حال چاپ باشد و یکی از اندستاپ‌ها فعال شود، چاپ متوقف می‌شود.
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "../config/mc_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// متغیرهای دیباگ (بند ۱۰.۸)
extern volatile uint32_t mc_dbg_475_esp_enable;    // نوشتنی: 1 = فعال، 0 = غیرفعال (پیش‌فرض از MC_ENDSTOP_PRINT_CHECK_DEFAULT)
extern volatile uint32_t mc_dbg_476_esp_trip_cnt;  // خواندنی: تعداد توقف‌ها به‌خاطر اندستاپ حین چاپ
extern volatile uint32_t mc_dbg_477_esp_last_mask; // خواندنی: ماسک اندستاپ آخرین توقف (بیت 0=X، 1=Y، 2=Z)
extern volatile uint32_t mc_dbg_488_esp_latched;   // خواندنی: 1 = توقف داده شده و تا رها شدن اندستاپ یا پایان حالت چاپ دوباره داده نمی‌شود (بند ۱۰.۸)

#ifdef __cplusplus
}

namespace mc {

constexpr uint32_t kEndstopPrintPollMs = 10U; // دوره بررسی زمان‌بند (میلی‌ثانیه)

// یک‌بار هنگام راه‌اندازی.
void mc_endstop_print_init();

// وظیفه زمان‌بند (هر 10 میلی‌ثانیه): اندستاپ‌ها را حین چاپ پایش می‌کند.
void mc_endstop_print_poll();

}  // namespace mc
#endif
