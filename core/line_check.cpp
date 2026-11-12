#include "line_check.h"
#include "hal_serial.h"

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_chk_result = 0;   // نتیجه آخرین بررسی: ۰ = درست، ۱ = چک‌سام غلط، ۲ = بدون چک‌سام، ۳ = چک‌سام بدون شماره خط، ۴ = ترتیب غلط
volatile int32_t  mc_dbg_chk_last_n = 0;   // شماره آخرین خط درست
volatile uint32_t mc_dbg_chk_errors = 0;   // تعداد درخواست‌های ارسال مجدد
}

namespace mc {
namespace {

int32_t g_last_n = 0;   // شماره آخرین خط درست

inline bool is_space(char c) { return (c == ' ') || (c == '\t'); }
inline bool is_digit(char c) { return (c >= '0') && (c <= '9'); }

// آیا متن با دستور M110 شروع می‌شود؟ (M110 و بعدش رقم نباشد)
bool is_m110(const char* t) {
    const char c = t[0];
    return ((c == 'M') || (c == 'm')) && (t[1] == '1') && (t[2] == '1') && (t[3] == '0') && !is_digit(t[4]);
}

uint8_t append_text(char* dst, uint8_t pos, const char* src) {
    while ((*src != '\0') && (pos < 100U)) {
        dst[pos] = *src;
        pos = static_cast<uint8_t>(pos + 1U);
        src = src + 1;
    }
    return pos;
}

uint8_t append_uint(char* dst, uint8_t pos, uint32_t value) {
    char digits[10];
    uint8_t d = 0U;
    do {
        digits[d] = static_cast<char>('0' + (value % 10U));
        d = static_cast<uint8_t>(d + 1U);
        value /= 10U;
    } while ((value > 0U) && (d < 10U));
    while ((d > 0U) && (pos < 100U)) {
        d = static_cast<uint8_t>(d - 1U);
        dst[pos] = digits[d];
        pos = static_cast<uint8_t>(pos + 1U);
    }
    return pos;
}

}  // namespace

int32_t mc_line_last_number() {
    return g_last_n;
}

LineCheck mc_line_check(const char* line, char* out, uint16_t out_size) {
    if ((out == nullptr) || (out_size == 0U)) { return LineCheck::Ok; }
    out[0] = '\0';
    if (line == nullptr) { return LineCheck::Ok; }

    // جای اولین * و پایان خط
    const char* star = nullptr;
    const char* end = line;
    while (*end != '\0') {
        if ((*end == '*') && (star == nullptr)) { star = end; }
        ++end;
    }
    const char* text_end = (star != nullptr) ? star : end;

    // شماره خط: N بزرگ و بعدش رقم، بعد از فاصله‌های اول خط
    const char* p = line;
    while (is_space(*p)) { ++p; }
    bool has_n = false;
    uint32_t n = 0U;
    const char* text = line;   // شروع متن دستور
    if ((*p == 'N') && is_digit(p[1])) {
        has_n = true;
        ++p;
        while (is_digit(*p)) {
            if (n <= 99999999U) { n = (n * 10U) + static_cast<uint32_t>(*p - '0'); }
            ++p;
        }
        while (is_space(*p)) { ++p; }
        text = p;
    }

    LineCheck result = LineCheck::Ok;
    if (has_n) {
        const bool m110 = is_m110(text);
        if ((static_cast<int32_t>(n) != (g_last_n + 1)) && !m110) {
            result = LineCheck::BadSequence;
        } else if (star == nullptr) {
            result = LineCheck::NoChecksum;
        } else {
            uint8_t sum = 0U;
            for (const char* q = line; q < star; ++q) {
                sum = static_cast<uint8_t>(sum ^ static_cast<uint8_t>(*q));
            }
            const char* c = star + 1;
            bool ok = is_digit(*c);
            uint32_t value = 0U;
            while (is_digit(*c)) {
                if (value <= 9999U) { value = (value * 10U) + static_cast<uint32_t>(*c - '0'); }
                ++c;
            }
            while (is_space(*c)) { ++c; }
            if (*c != '\0') { ok = false; }   // بعد از عدد چک‌سام چیز دیگری نباشد
            if (!ok || (value != sum)) { result = LineCheck::ChecksumMismatch; }
        }
        if (result == LineCheck::Ok) { g_last_n = static_cast<int32_t>(n); }
    } else if (star != nullptr) {
        result = LineCheck::NoLineNumber;
    }

    if (result == LineCheck::Ok) {
        uint16_t i = 0U;
        while ((text < text_end) && (i < static_cast<uint16_t>(out_size - 1U))) {
            out[i] = *text;
            ++i;
            ++text;
        }
        out[i] = '\0';
    }
    mc_dbg_chk_result = static_cast<uint32_t>(result);
    mc_dbg_chk_last_n = g_last_n;
    return result;
}

void mc_line_send_resend(LineCheck reason) {
    const char* what = nullptr;
    switch (reason) {
        case LineCheck::ChecksumMismatch: what = "checksum mismatch"; break;
        case LineCheck::NoChecksum:       what = "No Checksum with line number"; break;
        case LineCheck::NoLineNumber:     what = "No Line Number with checksum"; break;
        case LineCheck::BadSequence:      what = "Line Number is not Last Line Number+1"; break;
        default: return;
    }
    mc_dbg_chk_errors = mc_dbg_chk_errors + 1U;

    char buf[128];
    uint8_t n = 0U;
    n = append_text(buf, n, "Error:");
    n = append_text(buf, n, what);
    n = append_text(buf, n, ", Last Line: ");
    n = append_uint(buf, n, static_cast<uint32_t>(g_last_n));
    n = append_text(buf, n, "\nResend: ");
    n = append_uint(buf, n, static_cast<uint32_t>(g_last_n) + 1U);
    n = append_text(buf, n, "\nok\n");
    buf[n] = '\0';
    (void)mc_serial_print(buf);

    // بایت‌های منتظر را دور بریز تا میزبان از خط درخواست‌شده دوباره شروع کند
    uint8_t b = 0U;
    while (mc_serial_read(b)) { }
}

}  // namespace mc
