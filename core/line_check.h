#pragma once
// بررسی شماره خط (N) و چک‌سام (*) مثل Marlin، و درخواست ارسال مجدد.
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "line_reader.h"

namespace mc {

enum class LineCheck : uint8_t {
    Ok               = 0,   // خط درست است (یا شماره و چک‌سام نداشت)
    ChecksumMismatch = 1,   // چک‌سام با متن خط نمی‌خواند
    NoChecksum       = 2,   // شماره خط دارد ولی چک‌سام ندارد
    NoLineNumber     = 3,   // چک‌سام دارد ولی شماره خط ندارد
    BadSequence      = 4    // شماره خط یکی بیشتر از شماره قبلی نیست
};

// شماره آخرین خط درست (در شروع ۰).
int32_t mc_line_last_number();

// شماره آخرین خط درست را دستی می‌گذارد (برای دستور M110 N<عدد>).
void mc_line_set_last_number(int32_t n);

// یک خط کامل (خروجی mc_line_text) را بررسی می‌کند.
// اگر Ok باشد، متن دستور (بدون N و عدد آن، بدون * و چک‌سام) در out نوشته می‌شود.
// out باید حداقل kLineMaxChars + 1 بایت جا داشته باشد. اگر خطا باشد out خالی می‌ماند.
// خط بدون N و بدون * بدون تغییر در out می‌آید و شمارنده عوض نمی‌شود.
// خط N با دستور M110 از کنترل ترتیب معاف است و شمارنده را روی N خودش می‌گذارد.
LineCheck mc_line_check(const char* line, char* out, uint16_t out_size);

// پیام خطا را برای میزبان می‌فرستد: Error:<علت>, Last Line: n ، Resend: n+1 ، ok
// و بعد بایت‌های منتظر در صف دریافت را دور می‌ریزد (مثل Marlin).
// برای reason برابر Ok کاری نمی‌کند.
void mc_line_send_resend(LineCheck reason);

}  // namespace mc
