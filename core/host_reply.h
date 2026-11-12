#pragma once
// پاسخ‌ها به کامپیوتر (میزبان) مثل Marlin: ok ، خطا ، busy ، echo.
// فقط از hal_serial استفاده می‌کند. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

namespace mc {

// «ok» می‌فرستد.
void mc_reply_ok();
// «Error:<text>» و بعدش «ok» می‌فرستد.
void mc_reply_error(const char* text);
// «Error:Command failed: <حرف><شماره>» و بعدش «ok» (وقتی تابع یک دستور Error برگرداند).
void mc_reply_command_failed(char letter, uint16_t number);
// «echo:Unknown command: "<line>"» و بعدش «ok».
void mc_reply_unknown(const char* line);
// «echo:busy: processing» می‌فرستد (بدون ok).
void mc_reply_busy();
// یک خط متن (با خط جدید) برای کامپیوتر می‌فرستد، بدون ok. (ok را توزیع‌کننده بعد از تمام شدن دستور می‌فرستد.)
void mc_reply_line(const char* text);

}  // namespace mc
