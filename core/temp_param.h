#pragma once
// تبدیل پارامتر دما (درجه، float) به عدد صحیح ×۱۰ با گرد کردن و محدودکردن (بند ۴.۸).
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

namespace mc {

inline int32_t mc_temp_c_to_x10(float c) {
    if (!(c == c)) { return 0; }          // NaN
    if (c > 1000.0f)  { c = 1000.0f; }
    if (c < -100.0f)  { c = -100.0f; }
    float v = c * 10.0f;
    v = (v >= 0.0f) ? (v + 0.5f) : (v - 0.5f);
    return static_cast<int32_t>(v);
}

}  // namespace mc
