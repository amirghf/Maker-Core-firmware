#pragma once
// رابط لمس مستقل از برد. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
namespace mc {

// پین‌های لمس را راه‌اندازی می‌کند (CS خروجی و HIGH، پین PEN ورودی با پول‌آپ). true = موفق.
bool mc_touch_init();
// آیا الان صفحه لمس شده است؟ (پین PEN پایین باشد)
bool mc_touch_pressed();
// مختصات خام را (۰ تا ۴۰۹۵) می‌خواند. فقط اگر در تمام مدت خواندن صفحه لمس بوده true می‌دهد.
// اگر لمس نشده باشد یا ارسال SPI خطا بدهد false می‌دهد و x و y را عوض نمی‌کند.
bool mc_touch_read_raw(uint16_t* x, uint16_t* y);

}  // namespace mc
#endif
