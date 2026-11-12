// ارتباط USB مجازی (CDC) برای برد MW-X1.
// فقط پوشه boards مجاز به include کردن هدرهای ST است.
#include "hal_serial.h"
#include "hal_time.h"
#include "pins.h"
#include "board_info.h"                    // پوشه boards/mw_x1 در مسیر include است
#include "../../../config/mc_version.h"    // پوشه config در مسیر include نیست؛ مسیر نسبی از همین فایل
#include "usbd_cdc_if.h"                   // CDC_Transmit_FS و هدرهای USB

extern "C" {
extern USBD_HandleTypeDef hUsbDeviceFS;    // در usb_device.c تعریف شده

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
volatile int      mc_dbg_usb_connected = 0;  // 1 = پورت توسط کامپیوتر باز است
volatile uint32_t mc_dbg_usb_rx_total  = 0;  // کل بایت‌های دریافتی
volatile uint32_t mc_dbg_usb_rx_dropped = 0; // بایت‌هایی که جا نداشتند و دور ریخته شدند
}

namespace mc {
namespace {

constexpr uint16_t kRxSize      = 256;  // باید توان ۲ باشد
constexpr uint16_t kTxChunk     = 64;   // حداکثر اندازه هر بخش ارسال
constexpr uint32_t kTxTimeoutMs = 50;   // حداکثر انتظار برای تمام شدن ارسال قبلی

uint8_t           g_rx[kRxSize];
volatile uint16_t g_rx_head = 0;        // فقط وقفه می‌نویسد
volatile uint16_t g_rx_tail = 0;        // فقط حلقه اصلی می‌خواند
volatile uint8_t  g_dtr     = 0;        // وضعیت DTR
uint8_t           g_tx[kTxChunk];       // بافر ارسال (تا تمام شدن ارسال نباید عوض شود)
bool              g_was_connected = false;

// آیا USB کاملاً تنظیم شده؟ (قبل از این نباید به CDC_Transmit_FS دست زد)
inline bool usb_configured() {
    return hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED;
}

}  // namespace

bool mc_serial_connected() {
    return usb_configured() && (g_dtr != 0U);
}

uint16_t mc_serial_available() {
    return static_cast<uint16_t>((g_rx_head - g_rx_tail) & (kRxSize - 1U));
}

bool mc_serial_read(uint8_t& out) {
    if (g_rx_tail == g_rx_head) { return false; }
    out = g_rx[g_rx_tail];
    g_rx_tail = static_cast<uint16_t>((g_rx_tail + 1U) & (kRxSize - 1U));
    return true;
}

bool mc_serial_write(const uint8_t* data, uint16_t len) {
    if (data == nullptr) { return false; }
    uint16_t sent = 0;
    while (sent < len) {
        if (!mc_serial_connected()) { return false; }
        USBD_CDC_HandleTypeDef* hcdc = static_cast<USBD_CDC_HandleTypeDef*>(hUsbDeviceFS.pClassData);
        if (hcdc == nullptr) { return false; }
        // صبر تا ارسال قبلی تمام شود (بافر g_tx در حال استفاده است)
        const uint32_t t0 = mc_millis();
        while (hcdc->TxState != 0U) {
            if (mc_time_elapsed_ms(t0, kTxTimeoutMs)) { return false; }
        }
        uint16_t n = static_cast<uint16_t>(len - sent);
        if (n > kTxChunk) { n = kTxChunk; }
        for (uint16_t i = 0; i < n; ++i) {
            g_tx[i] = data[sent + i];
        }
        if (CDC_Transmit_FS(g_tx, n) != USBD_OK) { return false; }
        sent = static_cast<uint16_t>(sent + n);
    }
    return true;
}

bool mc_serial_print(const char* text) {
    if (text == nullptr) { return false; }
    uint16_t len = 0;
    while (text[len] != '\0') { ++len; }
    return mc_serial_write(reinterpret_cast<const uint8_t*>(text), len);
}

void mc_serial_poll() {
    const bool now = mc_serial_connected();
    mc_dbg_usb_connected = now ? 1 : 0;

    // تازه وصل شده: پیام شروع را می‌فرستیم
    if (now && !g_was_connected) {
        mc_serial_print(MC_FIRMWARE_NAME " " MC_VERSION_STRING " | " MC_BOARD_NAME "\r\n");
    }
    g_was_connected = now;

    uint8_t b = 0;
    if (!now) {
        // کسی گوش نمی‌دهد؛ داده‌های دریافتی قدیمی را دور می‌ریزیم
        while (mc_serial_read(b)) { }
        return;
    }

    // بایت‌های دریافتی را اینجا برنمی‌داریم؛ خواندنشان کار core/line_reader است.
}

}  // namespace mc

extern "C" void mc_serial_rx_c(const uint8_t* data, uint32_t len) {
    // این تابع داخل وقفه USB است؛ باید خیلی کوتاه بماند.
    for (uint32_t i = 0; i < len; ++i) {
        const uint16_t next = static_cast<uint16_t>((mc::g_rx_head + 1U) & (mc::kRxSize - 1U));
        if (next == mc::g_rx_tail) {
            mc_dbg_usb_rx_dropped = mc_dbg_usb_rx_dropped + 1U;  // صف پر است
            continue;
        }
        mc::g_rx[mc::g_rx_head] = data[i];
        mc::g_rx_head = next;
        mc_dbg_usb_rx_total = mc_dbg_usb_rx_total + 1U;
    }
}

extern "C" void mc_serial_dtr_c(int dtr) {
    mc::g_dtr = (dtr != 0) ? 1U : 0U;
}

extern "C" void mc_serial_poll_c(void) {
    mc::mc_serial_poll();
}
