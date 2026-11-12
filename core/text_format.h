#pragma once
// ساخت متن در بافر ثابت، بدون printf و بدون حافظه پویا.
// dst = بافر، cap = اندازه کل بافر، pos = جای شروع نوشتن. جای بعدی را برمی‌گرداند.
// همیشه صفر انتها نوشته می‌شود و هیچ‌وقت بیرون از بافر نمی‌نویسد (اضافه‌ها بریده می‌شوند).
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

namespace mc {

uint16_t mc_fmt_text(char* dst, uint16_t cap, uint16_t pos, const char* src);
uint16_t mc_fmt_uint(char* dst, uint16_t cap, uint16_t pos, uint32_t value);
// عدد اعشاری با decimals رقم اعشار (۰ تا ۳؛ بیشتر از ۳ برابر ۳ می‌شود). مثل 12.35 یا -7.50
// اگر عدد بعد از گرد کردن صفر شود، علامت منفی نوشته نمی‌شود. قدر مطلق بیشتر از ۹۹۹۹۹۹ بریده می‌شود.
// عدد نامعتبر (NaN) به‌صورت nan نوشته می‌شود.
uint16_t mc_fmt_float(char* dst, uint16_t cap, uint16_t pos, float value, uint8_t decimals);

}  // namespace mc
