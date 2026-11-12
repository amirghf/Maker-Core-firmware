#pragma once
// نوع تابع‌های دستور و جدول دستورها. هر دستور G-code در یک فایل جدا در پوشه gcode است.
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "gcode_parser.h"

namespace mc {

// نتیجه اجرای یک دستور
enum class GcodeStatus : uint8_t {
    Done  = 0,   // دستور تمام شد
    Busy  = 1,   // هنوز کار دارد؛ دفعه بعد دوباره صدا زده شود و دستور بعدی برداشته نمی‌شود
    Error = 2    // دستور خطا داد (فعلاً فقط شمرده و لاگ می‌شود؛ پاسخ به کامپیوتر در بند ۳.۶ است)
};

// تابع یک دستور. first_call فقط بار اول (برای هر دستور) true است؛ بعد از آن اگر Busy برگردانده
// باشد دوباره با همان cmd و first_call = false صدا زده می‌شود.
// تابع نباید منتظر بماند (حلقه نزند)؛ برای انتظار باید Busy برگرداند.
using GcodeHandler = GcodeStatus (*)(const GcodeCommand& cmd, bool first_call);

struct GcodeEntry {
    char         letter;    // 'G' یا 'M' یا 'T'
    uint16_t     number;    // مثلاً ۲۸
    int8_t       sub;       // زیرکد (G38.2 ← ۲)، یا -۱ اگر زیرکد ندارد
    GcodeHandler handler;
};

// دستور را در جدول (gcode_table.cpp) پیدا می‌کند. nullptr = دستور ناشناس.
const GcodeEntry* mc_gcode_find(char letter, uint16_t number, int8_t sub);

// دستور تست این بند: M9999 (فایل m9999.cpp)
GcodeStatus mc_gcode_m9999(const GcodeCommand& cmd, bool first_call);

}  // namespace mc
