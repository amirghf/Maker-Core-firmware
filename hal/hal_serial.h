#pragma once
// رابط ارتباط با کامپیوتر (USB مجازی) مستقل از برد. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
namespace mc {

// آیا کامپیوتر پورت را باز کرده؟ (کابل وصل و تنظیم‌شده و DTR روشن)
bool mc_serial_connected();
// چند بایت دریافتی آماده خواندن است.
uint16_t mc_serial_available();
// یک بایت دریافتی را در out می‌گذارد. اگر چیزی نبود false.
bool mc_serial_read(uint8_t& out);
// داده را برای کامپیوتر می‌فرستد. مسدودکننده است (حداکثر ۵۰ میلی‌ثانیه برای هر بخش). true = همه فرستاده شد.
bool mc_serial_write(const uint8_t* data, uint16_t len);
// یک متن (تمام‌شونده با صفر) می‌فرستد.
bool mc_serial_print(const char* text);
// هر دور حلقه اصلی صدا زده شود: پیام شروع هنگام اتصال، و دور ریختن داده‌های دریافتی وقتی کسی وصل نیست.
void mc_serial_poll();

}  // namespace mc

extern "C" {
#endif

// پل‌ها برای کد C. دو تای اول از وقفه USB صدا زده می‌شوند.
void mc_serial_rx_c(const uint8_t* data, uint32_t len);   // داده دریافتی
void mc_serial_dtr_c(int dtr);                            // وضعیت DTR (۰ یا ۱)
void mc_serial_poll_c(void);                              // برای حلقه اصلی main.c

#ifdef __cplusplus
}
#endif
