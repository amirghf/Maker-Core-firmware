// سیستم لاگ. هیچ هدر ST اینجا include نمی‌شود.
#include "log.h"
#include "ring_buffer.h"
#include "hal_serial.h"

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile int      mc_dbg_log_level     = 2;  // سطح فعلی: 0=Error ۱=Warn ۲=Info ۳=Debug (با دیباگر می‌شود عوض کرد)
volatile uint32_t mc_dbg_log_count     = 0;  // تعداد خط‌های وارد‌شده به صف
volatile uint32_t mc_dbg_log_dropped   = 0;  // تعداد خط‌های دورریخته (صف پر بود)
volatile uint32_t mc_dbg_log_sent      = 0;  // تعداد خط‌های فرستاده‌شده به کامپیوتر
volatile uint32_t mc_dbg_log_send_fail = 0;  // تعداد خط‌هایی که ارسالشان ناموفق بود
}

namespace mc {
namespace {

constexpr uint8_t kLineMax = 64;  // حداکثر طول یک خط شامل \r\n و صفر انتها

struct LogLine {
    char text[kLineMax];
};

RingBuffer<LogLine, 16> g_lines;  // ۱۵ خط جا دارد

char level_char(LogLevel level) {
    switch (level) {
        case LogLevel::Error: return 'E';
        case LogLevel::Warn:  return 'W';
        case LogLevel::Info:  return 'I';
        default:              return 'D';
    }
}

// متن را از جای pos در dst کپی می‌کند و جای بعدی را برمی‌گرداند. ۳ خانه آخر برای \r و \n و صفر می‌ماند.
uint8_t append_text(char* dst, uint8_t pos, const char* src) {
    if (src == nullptr) { return pos; }
    while ((*src != '\0') && (pos < static_cast<uint8_t>(kLineMax - 3U))) {
        dst[pos] = *src;
        pos = static_cast<uint8_t>(pos + 1U);
        src = src + 1;
    }
    return pos;
}

void push_line(LogLevel level, const char* text, bool has_value, uint32_t value) {
    if (static_cast<int>(level) > mc_dbg_log_level) { return; }

    LogLine line = {};
    uint8_t n = 0;
    line.text[n++] = '[';
    line.text[n++] = level_char(level);
    line.text[n++] = ']';
    line.text[n++] = ' ';
    n = append_text(line.text, n, text);

    if (has_value) {
        n = append_text(line.text, n, " = ");
        char digits[10];
        uint8_t d = 0;
        do {
            digits[d++] = static_cast<char>('0' + (value % 10U));
            value = value / 10U;
        } while ((value != 0U) && (d < 10U));
        while ((d > 0U) && (n < static_cast<uint8_t>(kLineMax - 3U))) {
            d = static_cast<uint8_t>(d - 1U);
            line.text[n++] = digits[d];
        }
    }

    line.text[n++] = '\r';
    line.text[n++] = '\n';
    line.text[n]   = '\0';

    if (g_lines.push(line)) {
        mc_dbg_log_count = mc_dbg_log_count + 1U;
    } else {
        mc_dbg_log_dropped = mc_dbg_log_dropped + 1U;
    }
}

}  // namespace

void mc_log(LogLevel level, const char* text) {
    push_line(level, text, false, 0U);
}

void mc_log_value(LogLevel level, const char* text, uint32_t value) {
    push_line(level, text, true, value);
}

void mc_log_set_level(LogLevel level) {
    mc_dbg_log_level = static_cast<int>(level);
}

LogLevel mc_log_get_level() {
    return static_cast<LogLevel>(mc_dbg_log_level);
}

void mc_log_flush() {
    if (!mc_serial_connected()) { return; }
    LogLine line = {};
    if (!g_lines.pop(line)) { return; }
    if (mc_serial_print(line.text)) {
        mc_dbg_log_sent = mc_dbg_log_sent + 1U;
    } else {
        mc_dbg_log_send_fail = mc_dbg_log_send_fail + 1U;
    }
}

}  // namespace mc
