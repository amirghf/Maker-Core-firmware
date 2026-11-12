#include "line_reader.h"
#include "hal_serial.h"
#include "log.h"
#include "host_reply.h"

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_line_count    = 0;  // تعداد خط‌های کامل دریافت‌شده
volatile uint32_t mc_dbg_line_len      = 0;  // طول آخرین خط کامل
volatile uint32_t mc_dbg_line_overflow = 0;  // تعداد خط‌های بلندتر از حد که کنار گذاشته شدند
}

namespace mc {
namespace {

char     g_line[kLineMaxChars + 1U];   // خط در حال ساخت (با صفر انتها)
uint16_t g_len      = 0;               // طول خط در حال ساخت
bool     g_overflow = false;           // خط فعلی از حد گذشته و باید دور ریخته شود
bool     g_ready    = false;           // یک خط کامل آماده برداشتن است

constexpr uint8_t kMaxBytesPerPoll = 128;   // سقف بایت خوانده‌شده در هر بار صدا زدن (تا وظیفه کوتاه بماند)

}  // namespace

void mc_line_poll() {
    // کسی وصل نیست: خط نیمه‌کاره را رها کن
    if (!mc_serial_connected()) {
        g_len = 0U;
        g_overflow = false;
        return;
    }
    // خط قبلی هنوز برداشته نشده
    if (g_ready) { return; }

    uint8_t b = 0;
    for (uint8_t i = 0U; i < kMaxBytesPerPoll; ++i) {
        if (!mc_serial_read(b)) { return; }

        if ((b == '\r') || (b == '\n')) {
            // پایان خط
            if (g_overflow) {
                mc_dbg_line_overflow = mc_dbg_line_overflow + 1U;
                mc_log(LogLevel::Error, "line too long");
                mc_reply_error("Line too long");
                g_overflow = false;
                g_len = 0U;
                continue;
            }
            if (g_len == 0U) { continue; }   // خط خالی (یا نیمه دوم CRLF)
            g_line[g_len] = '\0';
            mc_dbg_line_len = g_len;
            mc_dbg_line_count = mc_dbg_line_count + 1U;
            g_ready = true;
            return;
        }

        if ((b < 0x20U) || (b == 0x7FU)) { continue; }   // بایت کنترلی: نادیده بگیر

        if (g_overflow) { continue; }   // تا پایان خط دور می‌ریزیم
        if (g_len >= kLineMaxChars) {
            g_overflow = true;
            g_len = 0U;
            continue;
        }
        g_line[g_len] = static_cast<char>(b);
        g_len = static_cast<uint16_t>(g_len + 1U);
    }
}

bool mc_line_ready() {
    return g_ready;
}

const char* mc_line_text() {
    return g_line;
}

uint16_t mc_line_length() {
    return g_len;
}

void mc_line_release() {
    g_ready = false;
    g_len = 0U;
}

}  // namespace mc
