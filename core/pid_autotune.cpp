// تنظیم خودکار PID هیتر اکسترودر با روش رله (بند ۴.۹). هیچ هدر ST اینجا include نمی‌شود.
#include "pid_autotune.h"
#include "hal_time.h"

// متغیرهای debug؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_42_at_state      = 0;   // حالت: 0=Idle 1=Heating 2=RelayLow 3=RelayHigh 4=Done 5=Failed
volatile uint32_t mc_dbg_43_at_cycle      = 0;   // تعداد چرخه‌های کامل‌شده
volatile uint32_t mc_dbg_44_at_bias       = 0;   // bias فعلی (۰ تا ۲۵۵)
volatile uint32_t mc_dbg_45_at_period_x10 = 0;   // دوره آخرین چرخه ×۱۰ (50 = 5.0 ثانیه)
volatile uint32_t mc_dbg_46_at_amp_x100   = 0;   // دامنه آخرین چرخه ×۱۰۰ درجه (250 = 2.50 درجه)
volatile uint32_t mc_dbg_47_at_kp_x100    = 0;   // Kp آخرین چرخه ×۱۰۰
volatile uint32_t mc_dbg_48_at_ki_x100    = 0;   // Ki آخرین چرخه ×۱۰۰
volatile uint32_t mc_dbg_49_at_kd_x100    = 0;   // Kd آخرین چرخه ×۱۰۰
volatile uint32_t mc_dbg_50_at_fail       = 0;   // دلیل شکست: 0=ندارد 1=OverTemp 2=Timeout 3=Sensor 4=GainsInvalid
volatile uint32_t mc_dbg_51_at_power      = 0;   // توانی که تنظیم‌کننده به هیتر می‌دهد (۰ تا ۲۵۵)
volatile uint32_t mc_dbg_52_at_timeout_s  = 0;   // برای تست: زمان کل تنظیم به ثانیه؛ ۰ = پیش‌فرض (1200)، حداکثر 3600
}

namespace mc {
namespace {

constexpr float    kPi              = 3.14159265f;
constexpr uint32_t kTimeoutDefaultS = 1200U;    // زمان کل پیش‌فرض (۲۰ دقیقه)
constexpr uint32_t kMinPhaseMs      = 3000U;    // کمترین مدت هر فاز رله
constexpr int32_t  kHystX10         = 10;       // هیسترزیس رله ±۱ درجه
constexpr int32_t  kOverTempX10     = 200;      // بیشتر از ۲۰ درجه بالای هدف = شکست
constexpr float    kBiasStart       = 128.0f;
constexpr float    kBiasMin         = 20.0f;
constexpr float    kBiasMax         = 235.0f;
constexpr float    kKpMin           = 0.1f;
constexpr float    kKpMax           = 1000.0f;

AutotuneState  g_state        = AutotuneState::Idle;
AutotuneFail   g_fail         = AutotuneFail::None;
int32_t        g_target_x10   = 0;
uint8_t        g_cycles_total = 0U;
uint8_t        g_cycle        = 0U;
uint32_t       g_start_ms     = 0U;
uint32_t       g_phase_ms     = 0U;    // شروع فاز فعلی
uint32_t       g_low_ms       = 0U;    // مدت فاز RelayLow در چرخه جاری
int32_t        g_max_x10      = 0;
int32_t        g_min_x10      = 0;
float          g_bias         = kBiasStart;
float          g_d            = 0.0f;
uint8_t        g_power_low    = 0U;
uint8_t        g_power_high   = 0U;
uint8_t        g_power        = 0U;
AutotuneResult g_result       = {0.0f, 0.0f, 0.0f};

void set_state(AutotuneState s) {
    g_state = s;
    mc_dbg_42_at_state = static_cast<uint32_t>(s);
}

uint32_t to_x100(float v) {
    if (!(v > 0.0f)) { return 0U; }
    if (v > 4000000.0f) { v = 4000000.0f; }
    return static_cast<uint32_t>((v * 100.0f) + 0.5f);
}

// d و سطح‌های توان را از bias می‌سازد
void update_levels() {
    g_d = (g_bias > 127.5f) ? (255.0f - g_bias) : g_bias;
    g_power_high = static_cast<uint8_t>(g_bias + g_d + 0.5f);
    g_power_low  = static_cast<uint8_t>(g_bias - g_d + 0.5f);
    mc_dbg_44_at_bias = static_cast<uint32_t>(g_bias + 0.5f);
}

uint8_t fail(AutotuneFail f) {
    g_fail  = f;
    g_power = 0U;
    mc_dbg_50_at_fail  = static_cast<uint32_t>(f);
    mc_dbg_51_at_power = 0U;
    set_state(AutotuneState::Failed);
    return 0U;
}

void start_low_phase(uint32_t now, int32_t t_x10) {
    set_state(AutotuneState::RelayLow);
    g_phase_ms = now;
    g_max_x10  = t_x10;
    g_min_x10  = t_x10;
}

// یک چرخه کامل شد: ضرایب را حساب کن و bias را برای چرخه بعد اصلاح کن
void finish_cycle(uint32_t low_ms, uint32_t high_ms) {
    g_cycle = static_cast<uint8_t>(g_cycle + 1U);
    mc_dbg_43_at_cycle = g_cycle;

    const float total_ms = static_cast<float>(low_ms) + static_cast<float>(high_ms);
    const float pu_s     = total_ms / 1000.0f;
    const float amp_c    = static_cast<float>(g_max_x10 - g_min_x10) / 20.0f;   // نصف اختلاف قله و دره (×۱۰ به درجه)
    mc_dbg_45_at_period_x10 = static_cast<uint32_t>((pu_s * 10.0f) + 0.5f);
    mc_dbg_46_at_amp_x100   = to_x100(amp_c);

    bool ok = false;
    float kp = 0.0f;
    float ki = 0.0f;
    float kd = 0.0f;
    if ((amp_c > 0.0f) && (pu_s > 0.0f)) {
        const float ku = (4.0f * g_d) / (kPi * amp_c);
        kp = 0.6f * ku;
        ki = 1.2f * ku / pu_s;
        kd = 0.075f * ku * pu_s;
        ok = (kp >= kKpMin) && (kp <= kKpMax) && (ki > 0.0f) && (kd > 0.0f)
             && (ki < 1000000.0f) && (kd < 1000000.0f);   // مقایسه با NaN همیشه false است
    }
    if (!ok) {
        (void)fail(AutotuneFail::GainsInvalid);
        return;
    }
    g_result.kp = kp;
    g_result.ki = ki;
    g_result.kd = kd;
    mc_dbg_47_at_kp_x100 = to_x100(kp);
    mc_dbg_48_at_ki_x100 = to_x100(ki);
    mc_dbg_49_at_kd_x100 = to_x100(kd);

    // اصلاح bias: اگر فاز بالا طولانی‌تر بود، توان متوسط کم است و bias باید بالا برود
    const float imbalance = (static_cast<float>(high_ms) - static_cast<float>(low_ms)) / total_ms;
    g_bias += g_d * imbalance;
    if (g_bias < kBiasMin) { g_bias = kBiasMin; }
    if (g_bias > kBiasMax) { g_bias = kBiasMax; }
    update_levels();

    if (g_cycle >= g_cycles_total) {
        set_state(AutotuneState::Done);
    }
}

}  // namespace

void mc_autotune_begin(int32_t target_x10, uint8_t cycles) {
    g_target_x10   = target_x10;
    g_cycles_total = cycles;
    g_cycle        = 0U;
    g_fail         = AutotuneFail::None;
    g_result       = {0.0f, 0.0f, 0.0f};
    g_bias         = kBiasStart;
    update_levels();
    g_start_ms = mc_millis();
    g_phase_ms = g_start_ms;
    g_low_ms   = 0U;
    g_max_x10  = 0;
    g_min_x10  = 0;
    g_power    = 0U;

    mc_dbg_43_at_cycle      = 0U;
    mc_dbg_45_at_period_x10 = 0U;
    mc_dbg_46_at_amp_x100   = 0U;
    mc_dbg_47_at_kp_x100    = 0U;
    mc_dbg_48_at_ki_x100    = 0U;
    mc_dbg_49_at_kd_x100    = 0U;
    mc_dbg_50_at_fail       = 0U;
    mc_dbg_51_at_power      = 0U;
    set_state(AutotuneState::Heating);
}

uint8_t mc_autotune_step(bool have_temp, int32_t t_x10) {
    if (!mc_autotune_active()) { return 0U; }
    if (!have_temp) { return fail(AutotuneFail::Sensor); }

    const uint32_t now = mc_millis();
    uint32_t timeout_s = mc_dbg_52_at_timeout_s;
    if (timeout_s == 0U) { timeout_s = kTimeoutDefaultS; }
    if (timeout_s > 3600U) { timeout_s = 3600U; }
    if (mc_time_elapsed_ms(g_start_ms, timeout_s * 1000U)) { return fail(AutotuneFail::Timeout); }
    if (t_x10 >= (g_target_x10 + kOverTempX10)) { return fail(AutotuneFail::OverTemp); }

    if (t_x10 > g_max_x10) { g_max_x10 = t_x10; }
    if (t_x10 < g_min_x10) { g_min_x10 = t_x10; }

    switch (g_state) {
    case AutotuneState::Heating:
        g_power = 255U;
        if (t_x10 >= g_target_x10) {
            start_low_phase(now, t_x10);
            g_power = g_power_low;
        }
        break;

    case AutotuneState::RelayLow:
        g_power = g_power_low;
        if (mc_time_elapsed_ms(g_phase_ms, kMinPhaseMs) && (t_x10 <= (g_target_x10 - kHystX10))) {
            g_low_ms   = now - g_phase_ms;
            g_phase_ms = now;
            set_state(AutotuneState::RelayHigh);
            g_power = g_power_high;
        }
        break;

    case AutotuneState::RelayHigh:
        g_power = g_power_high;
        if (mc_time_elapsed_ms(g_phase_ms, kMinPhaseMs) && (t_x10 >= (g_target_x10 + kHystX10))) {
            const uint32_t high_ms = now - g_phase_ms;
            finish_cycle(g_low_ms, high_ms);          // ممکن است Done یا Failed شود
            if (g_state == AutotuneState::RelayHigh) {
                start_low_phase(now, t_x10);
                g_power = g_power_low;
            }
        }
        break;

    default:
        g_power = 0U;
        break;
    }

    if (!mc_autotune_active()) { g_power = 0U; }   // Done یا Failed: هیتر خاموش
    mc_dbg_51_at_power = g_power;
    return g_power;
}

bool mc_autotune_active() {
    return (g_state == AutotuneState::Heating) || (g_state == AutotuneState::RelayLow)
           || (g_state == AutotuneState::RelayHigh);
}

int32_t mc_autotune_target_x10() {
    return mc_autotune_active() ? g_target_x10 : 0;
}

AutotuneState mc_autotune_state()      { return g_state; }
AutotuneFail  mc_autotune_fail()       { return g_fail; }
uint8_t mc_autotune_cycle()            { return g_cycle; }
uint8_t mc_autotune_cycles_total()     { return g_cycles_total; }

bool mc_autotune_latest(AutotuneResult* out) {
    if ((out == nullptr) || (g_cycle == 0U)) { return false; }
    *out = g_result;
    return true;
}

void mc_autotune_release() {
    g_power = 0U;
    mc_dbg_51_at_power = 0U;
    set_state(AutotuneState::Idle);
}

}  // namespace mc
