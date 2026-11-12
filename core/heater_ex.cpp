// کنترل PID هیتر اکسترودر (بند ۴.۴). هیچ هدر ST اینجا include نمی‌شود.
#include "heater_ex.h"
#include "pid.h"
#include "thermistor.h"
#include "output_pwm.h"

// متغیرهای debug؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_hex_arm              = 0;      // برای تست: فقط وقتی ۱ باشد PID هیتر اکسترودر کار می‌کند
volatile int32_t  mc_dbg_hex_target_x10       = 0;      // برای تست: دمای هدف ×۱۰ (2000 = 200.0 درجه)؛ صفر یا منفی = خاموش
volatile uint32_t mc_dbg_hex_kp_x100          = 2000;   // برای تست: Kp ×۱۰۰ (2000 = 20.00)
volatile uint32_t mc_dbg_hex_ki_x100          = 100;    // برای تست: Ki ×۱۰۰ (100 = 1.00)
volatile uint32_t mc_dbg_hex_kd_x100          = 10000;  // برای تست: Kd ×۱۰۰ (10000 = 100.00)
volatile uint32_t mc_dbg_hex_state            = 0;      // وضعیت: 0=Off 1=SensorBad 2=FullOn 3=Active(PID) 4=CutOff
volatile uint32_t mc_dbg_hex_power            = 0;      // توان محاسبه‌شده توسط PID (۰ تا ۲۵۵)
volatile int32_t  mc_dbg_hex_target_applied_x10 = 0;    // هدفی که واقعاً اعمال می‌شود (بعد از سقف موقت)؛ ۰ وقتی خاموش است
volatile int32_t  mc_dbg_hex_i_x10            = 0;      // جمله انتگرال ×۱۰
volatile int32_t  mc_dbg_hex_d_x100           = 0;      // مشتق دما ×۱۰۰ (درجه بر ثانیه)
volatile int32_t  mc_dbg_hex_d_min_x100       = 0;      // کمترین (منفی‌ترین) مشتق دیده‌شده از آخرین مسلح‌شدن
volatile uint32_t mc_dbg_hex_power_max        = 0;      // بیشترین توان دیده‌شده از آخرین مسلح‌شدن
}

namespace mc {
namespace {

Pid g_pid;
constexpr float kPidDtS = 0.1f;   // برابر دوره وظیفه (۱۰۰ میلی‌ثانیه)

// توان صفر، پاک‌کردن PID و ثبت وضعیت
void set_off(HeaterExState st) {
    g_pid.reset();
    mc_output_set(OutputId::HeaterEx, 0);
    mc_dbg_hex_state  = static_cast<uint32_t>(st);
    mc_dbg_hex_power  = 0U;
    mc_dbg_hex_i_x10  = 0;
    mc_dbg_hex_d_x100 = 0;
}

}  // namespace

void mc_heater_ex_poll() {
    int32_t target = mc_dbg_hex_target_x10;
    if (target > kHeaterExTargetCapX10) { target = kHeaterExTargetCapX10; }

    // مسلح نیست یا هدف صفر: خاموش. شمارنده‌های «بیشینه» هم پاک می‌شوند تا هر بار مسلح‌شدن از نو شروع شود.
    if ((mc_dbg_hex_arm != 1U) || (target <= 0)) {
        mc_dbg_hex_target_applied_x10 = 0;
        mc_dbg_hex_d_min_x100 = 0;
        mc_dbg_hex_power_max  = 0U;
        set_off(HeaterExState::Off);
        return;
    }
    mc_dbg_hex_target_applied_x10 = target;

    int32_t t_x10 = 0;
    if (!mc_temp_get_x10(AdcChannel::TempExtruder, &t_x10)) {
        set_off(HeaterExState::SensorBad);   // سنسور سالم نیست: هیتر خاموش
        return;
    }

    PidGains g = {};
    g.kp = static_cast<float>(mc_dbg_hex_kp_x100) / 100.0f;
    g.ki = static_cast<float>(mc_dbg_hex_ki_x100) / 100.0f;
    g.kd = static_cast<float>(mc_dbg_hex_kd_x100) / 100.0f;

    PidRegion region = PidRegion::Active;
    const float out_f = g_pid.update(g, static_cast<float>(target) / 10.0f,
                                     static_cast<float>(t_x10) / 10.0f, kPidDtS, &region);
    const uint8_t power = static_cast<uint8_t>(out_f + 0.5f);
    mc_output_set(OutputId::HeaterEx, power);

    HeaterExState st = HeaterExState::Active;
    if (region == PidRegion::FullOn) { st = HeaterExState::FullOn; }
    if (region == PidRegion::CutOff) { st = HeaterExState::CutOff; }
    mc_dbg_hex_state = static_cast<uint32_t>(st);
    mc_dbg_hex_power = power;
    mc_dbg_hex_i_x10 = static_cast<int32_t>(g_pid.integral() * 10.0f);
    const int32_t d100 = static_cast<int32_t>(g_pid.derivative() * 100.0f);
    mc_dbg_hex_d_x100 = d100;
    if (d100 < mc_dbg_hex_d_min_x100) { mc_dbg_hex_d_min_x100 = d100; }
    if (power > mc_dbg_hex_power_max) { mc_dbg_hex_power_max = power; }
}

}  // namespace mc
