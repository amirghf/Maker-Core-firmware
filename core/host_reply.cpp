#include "host_reply.h"
#include "hal_serial.h"

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_reply_ok      = 0;   // تعداد ok های فرستاده‌شده
volatile uint32_t mc_dbg_reply_error   = 0;   // تعداد پیام‌های Error فرستاده‌شده
volatile uint32_t mc_dbg_reply_unknown = 0;   // تعداد پیام‌های دستور ناشناس
volatile uint32_t mc_dbg_reply_busy    = 0;   // تعداد پیام‌های busy
}

namespace mc {
namespace {

constexpr uint8_t kBufSize = 160;

uint8_t append_text(char* dst, uint8_t pos, const char* src) {
    if (src == nullptr) { return pos; }
    while ((*src != '\0') && (pos < static_cast<uint8_t>(kBufSize - 8U))) {
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
    while ((d > 0U) && (pos < static_cast<uint8_t>(kBufSize - 8U))) {
        d = static_cast<uint8_t>(d - 1U);
        dst[pos] = digits[d];
        pos = static_cast<uint8_t>(pos + 1U);
    }
    return pos;
}

// «ok» با خط جدید به انتهای بافر اضافه می‌کند، صفر انتها می‌گذارد و می‌فرستد.
void finish_with_ok_and_send(char* buf, uint8_t pos) {
    buf[pos] = 'o';
    buf[pos + 1U] = 'k';
    buf[pos + 2U] = '\n';
    buf[pos + 3U] = '\0';
    (void)mc_serial_print(buf);
    mc_dbg_reply_ok = mc_dbg_reply_ok + 1U;
}

}  // namespace

void mc_reply_ok() {
    (void)mc_serial_print("ok\n");
    mc_dbg_reply_ok = mc_dbg_reply_ok + 1U;
}

void mc_reply_ok_line(const char* text) {
    char buf[kBufSize];
    uint8_t n = 0U;
    n = append_text(buf, n, "ok ");
    n = append_text(buf, n, text);
    buf[n] = '\n';
    buf[n + 1U] = '\0';
    (void)mc_serial_print(buf);
    mc_dbg_reply_ok = mc_dbg_reply_ok + 1U;
}

void mc_reply_error(const char* text) {
    char buf[kBufSize];
    uint8_t n = 0U;
    n = append_text(buf, n, "Error:");
    n = append_text(buf, n, text);
    buf[n] = '\n';
    n = static_cast<uint8_t>(n + 1U);
    mc_dbg_reply_error = mc_dbg_reply_error + 1U;
    finish_with_ok_and_send(buf, n);
}

void mc_reply_command_failed(char letter, uint16_t number) {
    char buf[kBufSize];
    uint8_t n = 0U;
    n = append_text(buf, n, "Error:Command failed: ");
    buf[n] = letter;
    n = static_cast<uint8_t>(n + 1U);
    n = append_uint(buf, n, number);
    buf[n] = '\n';
    n = static_cast<uint8_t>(n + 1U);
    mc_dbg_reply_error = mc_dbg_reply_error + 1U;
    finish_with_ok_and_send(buf, n);
}

void mc_reply_unknown(const char* line) {
    char buf[kBufSize];
    uint8_t n = 0U;
    n = append_text(buf, n, "echo:Unknown command: \"");
    n = append_text(buf, n, line);
    n = append_text(buf, n, "\"");
    buf[n] = '\n';
    n = static_cast<uint8_t>(n + 1U);
    mc_dbg_reply_unknown = mc_dbg_reply_unknown + 1U;
    finish_with_ok_and_send(buf, n);
}

void mc_reply_busy() {
    (void)mc_serial_print("echo:busy: processing\n");
    mc_dbg_reply_busy = mc_dbg_reply_busy + 1U;
}

void mc_reply_line(const char* text) {
    char buf[kBufSize];
    uint8_t n = 0U;
    n = append_text(buf, n, text);
    buf[n] = '\n';
    buf[n + 1U] = '\0';
    (void)mc_serial_print(buf);
}

}  // namespace mc
