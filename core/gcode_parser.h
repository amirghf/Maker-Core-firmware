#pragma once
// تجزیه یک خط G-code به دستور و پارامترها. فقط محاسبه است و به سخت‌افزار دست نمی‌زند.
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "line_reader.h"

namespace mc {

constexpr uint8_t kGcodeMaxParams = 16;   // حداکثر تعداد پارامتر در یک خط

enum class GcodeResult : uint8_t {
    Ok            = 0,   // دستور درست تجزیه شد
    Empty         = 1,   // خط خالی بود یا فقط کامنت داشت (خطا نیست)
    BadCommand    = 2,   // کلمه دستور خراب است (با G یا M یا T شروع نشده، عدد ندارد، عدد بزرگ است)
    BadParam      = 3,   // پارامتر خراب است (کاراکتر نامعتبر، عدد ناقص مثل X- یا X1.2.3)
    TooManyParams = 4    // بیشتر از kGcodeMaxParams پارامتر
};

struct GcodeParam {
    char  letter;      // حرف بزرگ پارامتر، مثل 'X'
    bool  has_value;   // false = پارامتر بدون عدد (مثل X در G28 X)
    float value;       // مقدار (اگر has_value درست باشد)
};

// نتیجه تجزیه. همه چیز داخل خود ساختار است، پس بعد از برداشتن خط از خط‌خوان هم معتبر می‌ماند
// و می‌شود آن را کپی کرد (مثلاً داخل صف).
struct GcodeCommand {
    char       letter;                     // 'G' یا 'M' یا 'T'
    uint16_t   number;                     // مثلاً ۲۸ برای G28
    int8_t     sub;                        // رقم بعد از نقطه (G38.2 ← ۲)، و -۱ اگر نبود
    uint8_t    param_count;
    uint8_t    args_offset;                // جای شروع متن بعد از کلمه دستور داخل text
    GcodeParam params[kGcodeMaxParams];
    char       text[kLineMaxChars + 1U];   // متن خط بدون کامنت و بدون فاصله‌های انتها
};

// یک خط را تجزیه می‌کند. line باید با صفر تمام شود.
// قواعد: حروف کوچک بزرگ حساب می‌شوند. کامنت از ; شروع می‌شود. فاصله بین پارامترها لازم نیست
// (G1X10Y20 هم درست است). اگر یک حرف پارامتر دو بار بیاید، اولی می‌ماند.
// برای دستورهای متنی (M23، M28، M30، M32، M117، M118) پارامتر تجزیه نمی‌شود و فقط mc_gcode_args پر است.
// شماره خط (N) و چک‌سام (*) را اینجا تجزیه نمی‌کنیم؛ کارشان بند ۳.۳ است.
GcodeResult mc_gcode_parse(const char* line, GcodeCommand& out);

// آیا این پارامتر در خط هست؟ (چه با عدد چه بدون عدد)
bool mc_gcode_has(const GcodeCommand& cmd, char letter);
// آیا این پارامتر با عدد در خط هست؟
bool mc_gcode_has_value(const GcodeCommand& cmd, char letter);
// مقدار پارامتر، یا def اگر نبود یا بدون عدد بود.
float mc_gcode_value(const GcodeCommand& cmd, char letter, float def);
// مثل بالا ولی گردشده به نزدیک‌ترین عدد صحیح.
int32_t mc_gcode_int(const GcodeCommand& cmd, char letter, int32_t def);
// متن بعد از کلمه دستور (فاصله‌های اول حذف شده). برای M117 همان پیام است.
const char* mc_gcode_args(const GcodeCommand& cmd);

}  // namespace mc
