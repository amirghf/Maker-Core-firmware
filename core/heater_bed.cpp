// کنترل هیتر بستر (بند ۴.۵). هیچ هدر ST اینجا include نمی‌شود.
#include "heater_bed.h"
#include "thermistor.h"
#include "output_pwm.h"

// متغیرهای debug؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_06_hbed_arm             = 0;    // برای تست: فقط وقتی ۱ باشد کنترل بستر کار می‌کند
volatile int32_t  mc_dbg_04_hbed_target_x10      = 0;    // برای تست: دمای هدف ×۱۰ (600 = 60.0 درجه)؛ صفر یا منفی = خاموش
volatile int32_t  mc_dbg_hbed_target_applied_x10 = 0;    // هدفی که واقعاً اعمال می‌شود (بعد از سقف موقت)؛ ۰ وقتی خاموش است
volatile uint32_t mc_dbg_hbed_hyst_x10           = 20;   // برای تست: هیسترزیس ×۱۰ (20 = 2.0 درجه)
volatile uint32_t mc_dbg_15_hbed_state           = 0;    // وضعیت: 0=Off 1=SensorBad 2=Heating 3=Holding
volatile uint32_t mc_dbg_16_hbed_power           = 0;    // توان محاسبه‌شده (۰ یا ۲۵۵)
}

namespace mc {
namespace {

bool g_heating = false;   // حالت حافظه‌دار هیسترزیس: true = هیتر روشن

// توان صفر، پاک‌کردن حافظه هیسترزیس و ثبت وضعیت
void set_off(HeaterBedState st) {
    g_heating = false;
    mc_output_set(OutputId::HeaterBed, 0);
    mc_dbg_15_hbed_state = static_cast<uint32_t>(st);
    mc_dbg_16_hbed_power = 0U;
}

}  // namespace

void mc_heater_bed_poll() {
    int32_t target = mc_dbg_04_hbed_target_x10;
    if (target > kHeaterBedTargetCapX10) { target = kHeaterBedTargetCapX10; }

    // مسلح نیست یا هدف صفر: خاموش
    if ((mc_dbg_06_hbed_arm != 1U) || (target <= 0)) {
        mc_dbg_hbed_target_applied_x10 = 0;
        set_off(HeaterBedState::Off);
        return;
    }
    mc_dbg_hbed_target_applied_x10 = target;

    int32_t t_x10 = 0;
    if (!mc_temp_get_x10(AdcChannel::TempBed, &t_x10)) {
        set_off(HeaterBedState::SensorBad);   // سنسور سالم نیست: هیتر خاموش
        return;
    }

    int32_t hyst = static_cast<int32_t>(mc_dbg_hbed_hyst_x10);
    if (hyst > kHeaterBedHystMaxX10) { hyst = kHeaterBedHystMaxX10; }

    if (t_x10 >= (target + hyst)) {
        g_heating = false;
    } else if (t_x10 <= (target - hyst)) {
        g_heating = true;
    }   // بین دو حد: حالت قبلی حفظ می‌شود

    const uint8_t power = g_heating ? static_cast<uint8_t>(255U) : static_cast<uint8_t>(0U);
    mc_output_set(OutputId::HeaterBed, power);
    mc_dbg_15_hbed_state = static_cast<uint32_t>(g_heating ? HeaterBedState::Heating : HeaterBedState::Holding);
    mc_dbg_16_hbed_power = power;
}

bool mc_heater_bed_demanded() {
    return (mc_dbg_06_hbed_arm == 1U) && (mc_dbg_04_hbed_target_x10 > 0);
}

int32_t mc_heater_bed_target_x10() {
    if (mc_dbg_06_hbed_arm != 1U) { return 0; }
    int32_t target = mc_dbg_04_hbed_target_x10;
    if (target > kHeaterBedTargetCapX10) { target = kHeaterBedTargetCapX10; }
    return (target > 0) ? target : 0;
}

}  // namespace mc
