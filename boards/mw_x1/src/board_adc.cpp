// پیاده‌سازی ADC برای برد MW-X1 (ADC1: اکسترودر PC1 = کانال 11 ، بستر PC0 = کانال 10).
// فقط پوشه boards مجاز به include کردن هدرهای ST است.
#include "hal_adc.h"
#include "hal_time.h"
#include "pins.h"   // stm32f4xx_hal.h را هم می‌آورد
#include "adc.h"    // hadc1 که CubeMX ساخته

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile int      mc_adc_selftest_ok = 0;       // 1 = سلف‌تست پاس شد
volatile uint16_t mc_dbg_adc_ext_raw = 0xFFFF;  // مقدار خام اکسترودر (۰ تا ۴۰۹۵)
volatile uint16_t mc_dbg_adc_bed_raw = 0xFFFF;  // مقدار خام بستر (۰ تا ۴۰۹۵)
}

namespace mc {
namespace {

// ترتیب دقیقاً مثل enum class AdcChannel
constexpr uint32_t kChannels[] = { ADC_CHANNEL_11, ADC_CHANNEL_10 };

bool g_pins_ready = false;

// هر دو پین سنسور دما باید حالت آنالوگ باشند (بدون پول‌آپ داخلی).
void ensure_pins() {
    if (g_pins_ready) { return; }
    GPIO_InitTypeDef init = {};
    init.Mode = GPIO_MODE_ANALOG;
    init.Pull = GPIO_NOPULL;
    init.Pin = MC_SENSOR_TEMP_EX_PIN;
    HAL_GPIO_Init(MC_SENSOR_TEMP_EX_PORT, &init);
    init.Pin = MC_SENSOR_TEMP_BED_PIN;
    HAL_GPIO_Init(MC_SENSOR_TEMP_BED_PORT, &init);
    g_pins_ready = true;
}

}  // namespace

bool mc_adc_read(AdcChannel ch, uint16_t* raw12) {
    if (raw12 == nullptr) { return false; }
    const unsigned idx = static_cast<unsigned>(ch);
    if (idx >= static_cast<unsigned>(AdcChannel::Count)) { return false; }
    ensure_pins();

    ADC_ChannelConfTypeDef cfg = {};
    cfg.Channel      = kChannels[idx];
    cfg.Rank         = 1;
    cfg.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    if (HAL_ADC_ConfigChannel(&hadc1, &cfg) != HAL_OK) { return false; }
    if (HAL_ADC_Start(&hadc1) != HAL_OK) { return false; }
    const bool ok = (HAL_ADC_PollForConversion(&hadc1, 10U) == HAL_OK);
    if (ok) { *raw12 = static_cast<uint16_t>(HAL_ADC_GetValue(&hadc1)); }
    HAL_ADC_Stop(&hadc1);
    return ok;
}

bool mc_adc_selftest() {
    uint16_t ext = 0xFFFF;
    uint16_t bed = 0xFFFF;
    const bool ok_ext = mc_adc_read(AdcChannel::TempExtruder, &ext);
    const bool ok_bed = mc_adc_read(AdcChannel::TempBed, &bed);
    mc_dbg_adc_ext_raw = ext;
    mc_dbg_adc_bed_raw = bed;
    const bool pass = ok_ext && ok_bed && (ext <= 4095U) && (bed <= 4095U);
    mc_adc_selftest_ok = pass ? 1 : 0;
    return pass;
}

void mc_adc_debug_poll() {
    static uint32_t last_ms = 0;
    if (!mc_time_elapsed_ms(last_ms, 500U)) { return; }
    last_ms = mc_millis();
    uint16_t v = 0;
    if (mc_adc_read(AdcChannel::TempExtruder, &v)) { mc_dbg_adc_ext_raw = v; }
    if (mc_adc_read(AdcChannel::TempBed, &v))      { mc_dbg_adc_bed_raw = v; }
}

}  // namespace mc

extern "C" int mc_adc_selftest_c(void) {
    return mc::mc_adc_selftest() ? 1 : 0;
}

extern "C" void mc_adc_debug_poll_c(void) {
    mc::mc_adc_debug_poll();
}
