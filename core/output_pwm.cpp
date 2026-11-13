// PWM نرم‌افزاری هیتر و فن (بند ۴.۳). هیچ هدر ST اینجا include نمی‌شود.
#include "output_pwm.h"
#include "hal_gpio.h"
#include "hal_time.h"
#include "error.h"

// متغیرهای debug؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_out_arm          = 0;  // برای تست: فقط وقتی ۱ باشد توان debug فن اعمال می‌شود، در غیر این صورت ۰ (بستر از بند ۴.۵ جدا شده)
volatile uint32_t mc_dbg_out_ex_power     = 0;  // از بند ۴.۴ دیگر استفاده نمی‌شود؛ هیتر اکسترودر را PID می‌گذارد
volatile uint32_t mc_dbg_out_fan_power    = 0;  // برای تست: توان فن (۰ تا ۲۵۵)
volatile uint32_t mc_dbg_out_ex_on_ticks  = 0;  // تعداد تیک روشن در آخرین دوره کامل اکسترودر (از ۱۰۰۰)
volatile uint32_t mc_dbg_out_bed_on_ticks = 0;  // تعداد تیک روشن در آخرین دوره کامل بستر (از ۱۰۰۰۰)
volatile uint32_t mc_dbg_out_fan_on_ticks = 0;  // تعداد تیک روشن در آخرین دوره کامل فن (از ۱۰۰)
volatile uint32_t mc_dbg_out_ex_pin       = 0;  // سطح واقعی پین هیتر اکسترودر (۱ = HIGH)
volatile uint32_t mc_dbg_out_bed_pin      = 0;  // سطح واقعی پین هیتر بستر
volatile uint32_t mc_dbg_out_fan_pin      = 0;  // سطح واقعی پین فن
volatile uint32_t mc_dbg_out_killed_seen  = 0;  // ۱ = وقفه حالت kill را دیده و همه را خاموش کرده
}

namespace mc {
namespace {

struct Channel {
    PinId             pin;
    uint16_t          cycle_ticks;
    volatile uint16_t target_on;   // چند تیک از هر دوره روشن باشد
    volatile uint8_t  power;       // آخرین توان (۰ تا ۲۵۵)
    uint16_t          counter;     // شمارنده داخل دوره (فقط وقفه)
    uint16_t          on_count;    // تیک‌های روشن این دوره (فقط وقفه)
    bool              level;       // سطح فعلی پین (فقط وقفه)
    volatile uint16_t last_on;     // تیک‌های روشن دوره قبل
};

// ترتیب باید با OutputId یکی باشد
Channel g_ch[static_cast<uint8_t>(OutputId::Count)] = {
    { PinId::HeaterEx,    kHeaterExCycleTicks,  0, 0, 0, 0, false, 0 },
    { PinId::HeaterBed,   kHeaterBedCycleTicks, 0, 0, 0, 0, false, 0 },
    { PinId::FanFilament, kFanCycleTicks,       0, 0, 0, 0, false, 0 },
};
constexpr uint8_t kCount = static_cast<uint8_t>(OutputId::Count);
bool g_killed_seen = false;

// داخل وقفه صدا زده می‌شود؛ باید خیلی کوتاه باشد (بدون float، بدون حلقه طولانی).
void tick_hook() {
    if (mc_is_killed()) {
        for (uint8_t i = 0; i < kCount; ++i) {
            mc_gpio_write(g_ch[i].pin, false);
            g_ch[i].level = false;
        }
        if (!g_killed_seen) {
            g_killed_seen = true;
            mc_dbg_out_ex_pin  = mc_gpio_read(g_ch[0].pin) ? 1U : 0U;
            mc_dbg_out_bed_pin = mc_gpio_read(g_ch[1].pin) ? 1U : 0U;
            mc_dbg_out_fan_pin = mc_gpio_read(g_ch[2].pin) ? 1U : 0U;
            mc_dbg_out_killed_seen = 1U;
        }
        return;
    }
    for (uint8_t i = 0; i < kCount; ++i) {
        Channel& c = g_ch[i];
        const bool want = (c.counter < c.target_on);
        if (want != c.level) {
            mc_gpio_write(c.pin, want);
            c.level = want;
        }
        if (want) { c.on_count = static_cast<uint16_t>(c.on_count + 1U); }
        c.counter = static_cast<uint16_t>(c.counter + 1U);
        if (c.counter >= c.cycle_ticks) {
            c.last_on  = c.on_count;
            c.on_count = 0;
            c.counter  = 0;
        }
    }
}

uint8_t clamp_power(uint32_t v) {
    return (v > 255U) ? static_cast<uint8_t>(255U) : static_cast<uint8_t>(v);
}

}  // namespace

void mc_output_set(OutputId id, uint8_t power) {
    const uint8_t i = static_cast<uint8_t>(id);
    if (i >= kCount) { return; }
    Channel& c = g_ch[i];
    const uint32_t on = ((static_cast<uint32_t>(power) * c.cycle_ticks) + 127U) / 255U;
    c.power     = power;
    c.target_on = static_cast<uint16_t>(on);
}

uint8_t mc_output_get(OutputId id) {
    const uint8_t i = static_cast<uint8_t>(id);
    if (i >= kCount) { return 0; }
    return g_ch[i].power;
}

void mc_output_all_off() {
    for (uint8_t i = 0; i < kCount; ++i) {
        mc_output_set(static_cast<OutputId>(i), 0);
    }
}

void mc_output_init() {
    mc_output_all_off();
    mc_time_set_tick_hook(tick_hook);
}

void mc_output_poll() {
    // بند ۴.۵: هیتر بستر را دیگر اینجا نمی‌نویسیم (فقط heater_bed.cpp). فقط فن فعلاً از debug می‌آید.
    if (mc_dbg_out_arm == 1U) {
        mc_output_set(OutputId::Fan, clamp_power(mc_dbg_out_fan_power));
    } else {
        mc_output_set(OutputId::Fan, 0);
    }
    mc_dbg_out_ex_on_ticks  = g_ch[0].last_on;
    mc_dbg_out_bed_on_ticks = g_ch[1].last_on;
    mc_dbg_out_fan_on_ticks = g_ch[2].last_on;
    mc_dbg_out_ex_pin  = mc_gpio_read(g_ch[0].pin) ? 1U : 0U;
    mc_dbg_out_bed_pin = mc_gpio_read(g_ch[1].pin) ? 1U : 0U;
    mc_dbg_out_fan_pin = mc_gpio_read(g_ch[2].pin) ? 1U : 0U;
}

}  // namespace mc
