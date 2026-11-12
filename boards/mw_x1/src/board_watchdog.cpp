// پیاده‌سازی Watchdog برای برد MW-X1 با IWDG (ساعت LSI حدود ۳۲ کیلوهرتز).
// فقط پوشه boards مجاز به include کردن هدرهای ST است.
#include "hal_watchdog.h"
#include "pins.h"   // stm32f4xx_hal.h را هم می‌آورد

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile int      mc_dbg_wdt_active     = 0;  // 1 = Watchdog روشن است
volatile int      mc_dbg_wdt_was_reset  = 0;  // 1 = آخرین ریست توسط Watchdog بود
volatile uint32_t mc_dbg_wdt_timeout_ms = 0;  // زمان تایم‌اوت درخواستی
}

namespace mc {
namespace {

bool g_active    = false;
bool g_was_reset = false;

// پیش‌تقسیم‌کننده ۶۴: ساعت LSI حدود ۳۲ کیلوهرتز می‌شود حدود ۵۰۰ هرتز (هر شمارش حدود ۲ میلی‌ثانیه).
// توجه: LSI در این چیپ تلورانس زیادی دارد، پس زمان واقعی می‌تواند حدود نصف تا یک‌ونیم برابر باشد.
constexpr uint32_t kPrescalerValue = 4U;   // مقدار رجیستر PR برای تقسیم بر ۶۴
constexpr uint32_t kMsPerTick      = 2U;
constexpr uint32_t kMinTimeoutMs   = 100U;
constexpr uint32_t kMaxTimeoutMs   = 8000U;

}  // namespace

void mc_watchdog_start(uint32_t timeout_ms) {
    if (g_active) { return; }

    // علت ریست قبلی را بخوان و پرچم‌ها را پاک کن
    g_was_reset = ((RCC->CSR & RCC_CSR_IWDGRSTF) != 0U);
    RCC->CSR |= RCC_CSR_RMVF;
    mc_dbg_wdt_was_reset = g_was_reset ? 1 : 0;

    // هنگام توقف CPU توسط دیباگر (بریک‌پوینت)، Watchdog هم بایستد
    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;

    if (timeout_ms < kMinTimeoutMs) { timeout_ms = kMinTimeoutMs; }
    if (timeout_ms > kMaxTimeoutMs) { timeout_ms = kMaxTimeoutMs; }
    mc_dbg_wdt_timeout_ms = timeout_ms;
    const uint32_t reload = timeout_ms / kMsPerTick;   // حداکثر ۴۰۰۰ ، کمتر از ۴۰۹۵ مجاز

    IWDG->KR = 0xCCCCU;            // شروع Watchdog (LSI هم خودکار روشن می‌شود)
    IWDG->KR = 0x5555U;            // اجازه نوشتن روی PR و RLR
    IWDG->PR = kPrescalerValue;
    IWDG->RLR = reload;

    // صبر کن تا مقدارها اعمال شوند (حداکثر ۱۰۰ میلی‌ثانیه)
    const uint32_t t0 = HAL_GetTick();
    while ((IWDG->SR != 0U) && ((HAL_GetTick() - t0) < 100U)) {
    }
    IWDG->KR = 0xAAAAU;            // شمارنده را با مقدار جدید پر کن

    g_active = true;
    mc_dbg_wdt_active = 1;
}

void mc_watchdog_feed() {
    if (!g_active) { return; }
    IWDG->KR = 0xAAAAU;
}

bool mc_watchdog_active() {
    return g_active;
}

bool mc_watchdog_was_reset() {
    return g_was_reset;
}

}  // namespace mc

extern "C" void mc_watchdog_start_c(uint32_t timeout_ms) {
    mc::mc_watchdog_start(timeout_ms);
}
