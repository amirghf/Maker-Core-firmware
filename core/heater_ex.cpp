// کنترل PID هیتر اکسترودر (بند ۴.۴ و ۴.۹). هیچ هدر ST اینجا include نمی‌شود.
#include "heater_ex.h"
#include "pid.h"
#include "pid_autotune.h"
#include "thermistor.h"
#include "output_pwm.h"

// متغیرهای debug؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_03_hex_arm           = 0;      // برای تست: فقط وقتی ۱ باشد PID هیتر اکسترودر کار می‌کند
volatile int32_t  mc_dbg_01_hex_target_x10    = 0;      // برای تست: دمای هدف ×۱۰ (2000 = 200.0 درجه)؛ صفر یا منفی = خاموش
volatile uint32_t mc_dbg_39_hex_kp_x100       = 2000;   // Kp ×۱۰۰ (2000 = 20.00)؛ M303 با U1 این را عوض می‌کند
volatile uint32_t mc_dbg_40_hex_ki_x100       = 100;    // Ki ×۱۰۰ (100 = 1.00)؛ M303 با U1 این را عوض می‌کند
volatile uint32_t mc_dbg_41_hex_kd_x100       = 10000;  // Kd ×۱۰۰ (10000 = 100.00)؛ M303 با U1 این را عوض می‌کند
volatile uint32_t mc_dbg_13_hex_state         = 0;      // وضعیت: 0=Off 1=SensorBad 2=FullOn 3=Active(PID) 4=CutOff 5=Tuning
volatile uint32_t mc_dbg_14_hex_power         = 0;      // توان محاسبه‌شده توسط PID (۰ تا ۲۵۵)
volatile int32_t  mc_dbg_hex_target_applied_x10 = 0;    // هدفی که واقعاً اعمال می‌شود (بعد از سقف موقت)؛ ۰ وقتی خاموش است
volatile int32_t  mc_dbg_hex_i_x10            = 0;      // جمله انتگرال ×۱۰
volatile int32_t  mc_dbg_hex_d_x100           = 0;      // مشتق دما ×۱۰۰ (درجه بر ثانیه)
volatile int32_t  mc_dbg_hex_d_min_x100       = 0;      // کمترین (منفی‌ترین) مشتق دیده‌شده از آخرین مسلح‌شدن
volatile uint32_t mc_dbg_hex_power_max        = 0;      // بیشترین توان دیده‌شده از آخرین مسلح‌شدن
volatile int32_t  mc_dbg_33_gc_ex_target_x10 = 0;      // هدف اکسترودر که با M104 یا M109 گذاشته شده (×۱۰)؛ ۰ = خاموش
}

namespace mc {
namespace {

Pid g_pid;
constexpr float kPidDtS = 0.1f;   // برابر دوره وظیفه (۱۰۰ میلی‌ثانیه)
int32_t g_gcode_target_x10 = 0;   // هدف G-code (×۱۰)

// هدف مؤثر: وسط تنظیم خودکار (M303) هدف تنظیم‌کننده، در حالت تست از debug، وگرنه از G-code. بعد سقف. صفر یعنی خاموش.
int32_t effective_target_x10() {
    int32_t t = 0;
    if (mc_autotune_active()) {
        t = mc_autotune_target_x10();
    } else {
        t = (mc_dbg_03_hex_arm == 1U) ? mc_dbg_01_hex_target_x10 : g_gcode_target_x10;
    }
    if (t > kHeaterExTargetCapX10) { t = kHeaterExTargetCapX10; }
    return (t > 0) ? t : 0;
}

// توان صفر، پاک‌کردن PID و ثبت وضعیت
void set_off(HeaterExState st) {
    g_pid.reset();
    mc_output_set(OutputId::HeaterEx, 0);
    mc_dbg_13_hex_state = static_cast<uint32_t>(st);
    mc_dbg_14_hex_power = 0U;
    mc_dbg_hex_i_x10    = 0;
    mc_dbg_hex_d_x100   = 0;
}

}  // namespace

void mc_heater_ex_poll() {
    mc_dbg_33_gc_ex_target_x10 = g_gcode_target_x10;

    // تنظیم خودکار PID (M303، بند ۴.۹): توان را تنظیم‌کننده می‌گذارد و PID عادی کار نمی‌کند.
    // حفاظت‌های ۴.۶ و ۴.۷ همچنان فعال‌اند (هدف تنظیم از mc_heater_ex_target_x10 دیده می‌شود).
    if (mc_autotune_active()) {
        int32_t ta = 0;
        const bool ok_a = mc_temp_get_x10(AdcChannel::TempExtruder, &ta);
        const uint8_t pa = mc_autotune_step(ok_a, ta);
        mc_output_set(OutputId::HeaterEx, pa);
        mc_dbg_13_hex_state = static_cast<uint32_t>(HeaterExState::Tuning);
        mc_dbg_14_hex_power = pa;
        mc_dbg_hex_target_applied_x10 = mc_autotune_target_x10();
        g_pid.reset();
        return;
    }

    const int32_t target = effective_target_x10();

    // هدف صفر: خاموش. شمارنده‌های «بیشینه» هم پاک می‌شوند تا هر بار روشن‌شدن از نو شروع شود.
    if (target <= 0) {
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
    g.kp = static_cast<float>(mc_dbg_39_hex_kp_x100) / 100.0f;
    g.ki = static_cast<float>(mc_dbg_40_hex_ki_x100) / 100.0f;
    g.kd = static_cast<float>(mc_dbg_41_hex_kd_x100) / 100.0f;

    PidRegion region = PidRegion::Active;
    const float out_f = g_pid.update(g, static_cast<float>(target) / 10.0f,
                                     static_cast<float>(t_x10) / 10.0f, kPidDtS, &region);
    const uint8_t power = static_cast<uint8_t>(out_f + 0.5f);
    mc_output_set(OutputId::HeaterEx, power);

    HeaterExState st = HeaterExState::Active;
    if (region == PidRegion::FullOn) { st = HeaterExState::FullOn; }
    if (region == PidRegion::CutOff) { st = HeaterExState::CutOff; }
    mc_dbg_13_hex_state = static_cast<uint32_t>(st);
    mc_dbg_14_hex_power = power;
    mc_dbg_hex_i_x10 = static_cast<int32_t>(g_pid.integral() * 10.0f);
    const int32_t d100 = static_cast<int32_t>(g_pid.derivative() * 100.0f);
    mc_dbg_hex_d_x100 = d100;
    if (d100 < mc_dbg_hex_d_min_x100) { mc_dbg_hex_d_min_x100 = d100; }
    if (power > mc_dbg_hex_power_max) { mc_dbg_hex_power_max = power; }
}

bool mc_heater_ex_demanded() {
    return effective_target_x10() > 0;
}

int32_t mc_heater_ex_target_x10() {
    return effective_target_x10();
}

bool mc_heater_ex_test_mode() {
    return mc_dbg_03_hex_arm == 1U;
}

int32_t mc_heater_ex_set_target_x10(int32_t t_x10) {
    if (t_x10 < 0) { t_x10 = 0; }
    if (t_x10 > kHeaterExTargetCapX10) { t_x10 = kHeaterExTargetCapX10; }
    g_gcode_target_x10 = t_x10;
    mc_dbg_33_gc_ex_target_x10 = t_x10;
    return t_x10;
}

void mc_heater_ex_set_gains_x100(uint32_t kp_x100, uint32_t ki_x100, uint32_t kd_x100) {
    mc_dbg_39_hex_kp_x100 = kp_x100;
    mc_dbg_40_hex_ki_x100 = ki_x100;
    mc_dbg_41_hex_kd_x100 = kd_x100;
}

}  // namespace mc
