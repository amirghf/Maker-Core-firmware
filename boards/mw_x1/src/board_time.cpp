// زمان‌پایه برد MW-X1 بر پایه TIM6 (شمارش ۱ مگاهرتز، وقفه هر ۱۰۰ میکروثانیه).
// تنظیم تایمر در CubeMX است (Prescaler=83 ، Period=99 ، کلاک تایمر ۸۴ مگاهرتز).
// فقط پوشه boards مجاز به include کردن هدرهای ST است.
#include "hal_time.h"
#include "pins.h"   // stm32f4xx_hal.h را هم می‌آورد
#include "tim.h"    // htim6 که CubeMX ساخته

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile int      mc_time_selftest_ok = 0;  // 1 = سلف‌تست زمان پاس شد
volatile uint32_t mc_dbg_millis = 0;
volatile uint32_t mc_dbg_micros = 0;
volatile uint32_t mc_dbg_ticks  = 0;
}

namespace mc {
namespace {

constexpr uint32_t kTickUs = 100;  // هر تیک ۱۰۰ میکروثانیه
volatile uint32_t g_ticks = 0;     // فقط وقفه TIM6 این را زیاد می‌کند

}  // namespace

void mc_time_init() {
    g_ticks = 0;
    __HAL_TIM_SET_COUNTER(&htim6, 0);
    __HAL_TIM_CLEAR_FLAG(&htim6, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start_IT(&htim6);
}

uint32_t mc_millis() {
    return HAL_GetTick();
}

uint32_t mc_time_ticks() {
    return g_ticks;
}

uint32_t mc_micros() {
    uint32_t t1;
    uint32_t t2;
    uint32_t cnt;
    uint32_t sr;
    // ترتیب خواندن مهم است: تیک، شمارنده، پرچم، دوباره تیک. اگر وقفه وسط کار آمد، دوباره می‌خوانیم.
    do {
        t1  = g_ticks;
        cnt = TIM6->CNT;
        sr  = TIM6->SR;
        t2  = g_ticks;
    } while (t1 != t2);
    // اگر شمارنده دور زده ولی وقفه هنوز سرویس نشده (پرچم UIF روشن و شمارنده کوچک)، یک تیک اضافه می‌کنیم
    if (((sr & TIM_SR_UIF) != 0U) && (cnt < (kTickUs / 2U))) {
        t1 = t1 + 1U;
    }
    return (t1 * kTickUs) + cnt;
}

bool mc_time_selftest() {
    // ۱) mc_micros هرگز نباید به عقب برود
    uint32_t prev = mc_micros();
    for (uint32_t i = 0; i < 2000U; ++i) {
        const uint32_t now = mc_micros();
        if (static_cast<int32_t>(now - prev) < 0) { return false; }
        prev = now;
    }

    // ۲) حدود ۱۰۰ میلی‌ثانیه صبر می‌کنیم (با mc_millis، نه HAL_Delay) و سه ساعت را مقایسه می‌کنیم
    const uint32_t ms0 = mc_millis();
    const uint32_t us0 = mc_micros();
    const uint32_t tk0 = mc_time_ticks();
    uint32_t guard = 0;
    while ((mc_millis() - ms0) < 100U) {
        // اگر SysTick کار نکند نباید برد برای همیشه گیر کند
        guard = guard + 1U;
        if (guard > 5000000U) { return false; }
    }
    const uint32_t d_ms = mc_millis() - ms0;
    const uint32_t d_us = mc_micros() - us0;
    const uint32_t d_tk = mc_time_ticks() - tk0;

    // ۳) micros باید با millis جور باشد (mc_millis دقت ±۱ میلی‌ثانیه دارد، پس ±۲۵۰۰ میکروثانیه تحمل)
    const int32_t err_us = static_cast<int32_t>(d_us) - static_cast<int32_t>(d_ms * 1000U);
    if ((err_us > 2500) || (err_us < -2500)) { return false; }

    // ۴) تعداد تیک‌ها باید با micros جور باشد (تحمل ۳ تیک)
    const int32_t err_tk = static_cast<int32_t>(d_tk * kTickUs) - static_cast<int32_t>(d_us);
    if ((err_tk > 300) || (err_tk < -300)) { return false; }

    return true;
}

}  // namespace mc

extern "C" void mc_time_init_c(void) {
    mc::mc_time_init();
}

extern "C" int mc_time_selftest_c(void) {
    mc_time_selftest_ok = mc::mc_time_selftest() ? 1 : 0;
    return mc_time_selftest_ok;
}

extern "C" void mc_time_tick_c(void) {
    mc::g_ticks = mc::g_ticks + 1U;  // خیلی کوتاه بماند؛ این تابع داخل وقفه است
}

extern "C" void mc_time_debug_poll_c(void) {
    mc_dbg_millis = mc::mc_millis();
    mc_dbg_micros = mc::mc_micros();
    mc_dbg_ticks  = mc::mc_time_ticks();
}
