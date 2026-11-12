// پیاده‌سازی نمایشگر برای برد MW-X1: نمایشگر MKS TS35 V2 با تراشه ST7796S روی SPI1.
// فقط پوشه boards مجاز به include کردن هدرهای ST است.
#include "hal_display.h"
#include "hal_gpio.h"
#include "pins.h"   // stm32f4xx_hal.h را هم می‌آورد
#include "spi.h"    // hspi1 که CubeMX ساخته

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile int      mc_dbg_tft_init_ok   = -1;  // -1 = هنوز امتحان نشده، 1 = راه‌اندازی موفق، 0 = ارسال ناموفق
volatile uint32_t mc_dbg_tft_spi_fail  = 0;   // تعداد ارسال‌های SPI که خطا دادند
}

namespace mc {
namespace {

constexpr uint16_t kWidth        = 480U;
constexpr uint16_t kHeight       = 320U;
constexpr uint32_t kSpiTimeoutMs = 50U;
constexpr uint8_t  kMadctl       = 0x28U;  // افقی، ترتیب رنگ BGR (روی این برد تأیید شده)
constexpr uint16_t kChunkPixels  = 128U;   // تعداد پیکسل در هر بسته ارسال
constexpr uint32_t kSpiPrescaler = SPI_BAUDRATEPRESCALER_4;   // سرعت SPI نمایشگر (۲۱ مگاهرتز). اگر نویز دیده شد به SPI_BAUDRATEPRESCALER_8 تغییر می‌کند.

bool g_ready = false;

// یک بسته را روی SPI1 می‌فرستد. CS و RS را فراخواننده تنظیم می‌کند.
bool spi_send(const uint8_t* data, uint16_t len) {
    if (HAL_SPI_Transmit(&hspi1, const_cast<uint8_t*>(data), len, kSpiTimeoutMs) != HAL_OK) {
        mc_dbg_tft_spi_fail = mc_dbg_tft_spi_fail + 1U;
        return false;
    }
    return true;
}

// سرعت SPI1 را تنظیم می‌کند (فقط بیت‌های BR در CR1).
void set_spi_speed(uint32_t prescaler) {
    __HAL_SPI_DISABLE(&hspi1);
    MODIFY_REG(hspi1.Instance->CR1, SPI_CR1_BR, prescaler);
    hspi1.Init.BaudRatePrescaler = prescaler;
    __HAL_SPI_ENABLE(&hspi1);
}

// یک دستور و (اختیاری) داده‌هایش را می‌فرستد.
void write_cmd(uint8_t cmd, const uint8_t* data, uint16_t len) {
    mc_gpio_write(PinId::LcdCs, false);
    mc_gpio_write(PinId::LcdRs, false);   // RS پایین = دستور
    (void)spi_send(&cmd, 1U);
    if ((len > 0U) && (data != nullptr)) {
        mc_gpio_write(PinId::LcdRs, true); // RS بالا = داده
        (void)spi_send(data, len);
    }
    mc_gpio_write(PinId::LcdCs, true);
}

// یک مرحله از دنباله راه‌اندازی.
struct InitStep {
    uint8_t cmd;
    uint8_t len;
    uint8_t data[14];
    uint8_t delay_ms;   // مکث بعد از این مرحله
};

// دنباله راه‌اندازی ST7796S
constexpr InitStep kInit[] = {
    { 0x01U, 0U, {0}, 150U },                                   // ریست نرم‌افزاری
    { 0x11U, 0U, {0}, 150U },                                   // خروج از خواب
    { 0xF0U, 1U, { 0xC3U }, 0U },                               // باز کردن دستورهای بخش ۲ (بخش اول)
    { 0xF0U, 1U, { 0x96U }, 0U },                               // باز کردن دستورهای بخش ۲ (بخش دوم)
    { 0x36U, 1U, { kMadctl }, 0U },                             // جهت نمایش
    { 0x3AU, 1U, { 0x55U }, 0U },                               // فرمت پیکسل RGB565
    { 0xB4U, 1U, { 0x01U }, 0U },                               // وارونگی ستونی
    { 0xB6U, 3U, { 0x80U, 0x02U, 0x3BU }, 0U },                 // کنترل نمایش
    { 0xE8U, 8U, { 0x40U, 0x8AU, 0x00U, 0x00U, 0x29U, 0x19U, 0xA5U, 0x33U }, 0U },
    { 0xC1U, 1U, { 0x06U }, 0U },                               // کنترل توان
    { 0xC2U, 1U, { 0xA7U }, 0U },
    { 0xC5U, 1U, { 0x18U }, 120U },                             // VCOM
    { 0xE0U, 14U, { 0xF0U, 0x09U, 0x0BU, 0x06U, 0x04U, 0x15U, 0x2FU,
                    0x54U, 0x42U, 0x3CU, 0x17U, 0x14U, 0x18U, 0x1BU }, 0U },  // گامای مثبت
    { 0xE1U, 14U, { 0xE0U, 0x09U, 0x0BU, 0x06U, 0x04U, 0x03U, 0x2BU,
                    0x43U, 0x42U, 0x3BU, 0x16U, 0x14U, 0x17U, 0x1BU }, 120U }, // گامای منفی
    { 0xF0U, 1U, { 0x3CU }, 0U },                               // بستن دستورهای بخش ۲ (بخش اول)
    { 0xF0U, 1U, { 0x69U }, 120U },                             // بستن دستورهای بخش ۲ (بخش دوم)
    { 0x20U, 0U, {0}, 0U },                                     // inversion خاموش (رنگ‌ها روی نمایشگر واقعی همین‌طور درست شدند)
    { 0x29U, 0U, {0}, 50U }                                     // روشن کردن نمایش
};

// پنجره نوشتن را مشخص می‌کند و دستور شروع نوشتن پیکسل را می‌فرستد.
// بعد از آن CS پایین و RS بالا می‌ماند تا فراخواننده داده پیکسل بفرستد.
void begin_pixels(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    const uint8_t col[4] = {
        static_cast<uint8_t>(x0 >> 8), static_cast<uint8_t>(x0 & 0xFFU),
        static_cast<uint8_t>(x1 >> 8), static_cast<uint8_t>(x1 & 0xFFU)
    };
    const uint8_t row[4] = {
        static_cast<uint8_t>(y0 >> 8), static_cast<uint8_t>(y0 & 0xFFU),
        static_cast<uint8_t>(y1 >> 8), static_cast<uint8_t>(y1 & 0xFFU)
    };
    write_cmd(0x2AU, col, 4U);   // محدوده ستون
    write_cmd(0x2BU, row, 4U);   // محدوده سطر

    const uint8_t ramwr = 0x2CU;
    mc_gpio_write(PinId::LcdCs, false);
    mc_gpio_write(PinId::LcdRs, false);
    (void)spi_send(&ramwr, 1U);
    mc_gpio_write(PinId::LcdRs, true);
}

}  // namespace

bool mc_display_init() {
    g_ready = false;
    const uint32_t fail_before = mc_dbg_tft_spi_fail;

    // پین‌ها را خروجی می‌کنیم و در حالت امن می‌گذاریم: CS ها HIGH و بک‌لایت خاموش.
    mc_gpio_set_mode(PinId::LcdCs,   PinMode::Output);
    mc_gpio_set_mode(PinId::LcdRs,   PinMode::Output);
    mc_gpio_set_mode(PinId::LcdRes,  PinMode::Output);
    mc_gpio_set_mode(PinId::LcdBlk,  PinMode::Output);
    mc_gpio_set_mode(PinId::TouchCs, PinMode::Output);
    mc_gpio_write(PinId::LcdBlk,  false);
    mc_gpio_write(PinId::LcdCs,   true);
    mc_gpio_write(PinId::TouchCs, true);   // لمس روی همین SPI است؛ انتخاب نشود
    mc_gpio_write(PinId::LcdRs,   true);

    // ریست سخت‌افزاری
    mc_gpio_write(PinId::LcdRes, true);
    HAL_Delay(5U);
    mc_gpio_write(PinId::LcdRes, false);
    HAL_Delay(20U);
    mc_gpio_write(PinId::LcdRes, true);
    HAL_Delay(150U);

    set_spi_speed(kSpiPrescaler);

    for (const InitStep& s : kInit) {
        write_cmd(s.cmd, s.data, s.len);
        if (s.delay_ms > 0U) { HAL_Delay(s.delay_ms); }
    }

    const bool ok = (mc_dbg_tft_spi_fail == fail_before);
    mc_dbg_tft_init_ok = ok ? 1 : 0;
    g_ready = ok;
    return ok;
}

void mc_display_backlight(bool on) {
    mc_gpio_write(PinId::LcdBlk, on);
}

uint16_t mc_display_width() {
    return kWidth;
}

uint16_t mc_display_height() {
    return kHeight;
}

void mc_display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (!g_ready) { return; }
    if ((x >= kWidth) || (y >= kHeight) || (w == 0U) || (h == 0U)) { return; }
    if ((static_cast<uint32_t>(x) + w) > kWidth)  { w = static_cast<uint16_t>(kWidth - x); }
    if ((static_cast<uint32_t>(y) + h) > kHeight) { h = static_cast<uint16_t>(kHeight - y); }

    // یک بسته پر از همین رنگ (بایت بالا اول)
    uint8_t buf[kChunkPixels * 2U];
    for (uint16_t i = 0U; i < kChunkPixels; i = static_cast<uint16_t>(i + 1U)) {
        buf[2U * i]      = static_cast<uint8_t>(color >> 8);
        buf[(2U * i) + 1U] = static_cast<uint8_t>(color & 0xFFU);
    }

    begin_pixels(x, y, static_cast<uint16_t>(x + w - 1U), static_cast<uint16_t>(y + h - 1U));
    uint32_t remaining = static_cast<uint32_t>(w) * h;
    while (remaining > 0U) {
        const uint16_t n = (remaining > kChunkPixels) ? kChunkPixels : static_cast<uint16_t>(remaining);
        (void)spi_send(buf, static_cast<uint16_t>(n * 2U));
        remaining -= n;
    }
    mc_gpio_write(PinId::LcdCs, true);
}

void mc_display_fill_screen(uint16_t color) {
    mc_display_fill_rect(0U, 0U, kWidth, kHeight, color);
}

void mc_display_write_pixels(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* pixels) {
    if (!g_ready) { return; }
    if ((pixels == nullptr) || (w == 0U) || (h == 0U)) { return; }
    if ((static_cast<uint32_t>(x) + w) > kWidth)  { return; }
    if ((static_cast<uint32_t>(y) + h) > kHeight) { return; }

    uint8_t buf[kChunkPixels * 2U];
    begin_pixels(x, y, static_cast<uint16_t>(x + w - 1U), static_cast<uint16_t>(y + h - 1U));
    uint32_t remaining = static_cast<uint32_t>(w) * h;
    uint32_t index = 0U;
    while (remaining > 0U) {
        const uint16_t n = (remaining > kChunkPixels) ? kChunkPixels : static_cast<uint16_t>(remaining);
        for (uint16_t i = 0U; i < n; i = static_cast<uint16_t>(i + 1U)) {
            const uint16_t c = pixels[index + i];
            buf[2U * i]        = static_cast<uint8_t>(c >> 8);
            buf[(2U * i) + 1U] = static_cast<uint8_t>(c & 0xFFU);
        }
        (void)spi_send(buf, static_cast<uint16_t>(n * 2U));
        index += n;
        remaining -= n;
    }
    mc_gpio_write(PinId::LcdCs, true);
}

}  // namespace mc
