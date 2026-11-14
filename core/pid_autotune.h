#pragma once
// تنظیم خودکار PID هیتر اکسترودر با روش رله (بند ۴.۹). هیچ هدر ST اینجا include نمی‌شود.
// فقط محاسبه و ماشین حالت است؛ توان را برمی‌گرداند و خودش به خروجی دست نمی‌زند.
#include <stdint.h>

namespace mc {

enum class AutotuneState : uint8_t {
    Idle      = 0,   // کاری در جریان نیست
    Heating   = 1,   // گرم‌شدن اولیه با توان کامل تا رسیدن به هدف
    RelayLow  = 2,   // رله: توان پایین (دما باید پایین بیاید)
    RelayHigh = 3,   // رله: توان بالا (دما باید بالا برود)
    Done      = 4,   // تمام شد؛ نتیجه آماده است و منتظر mc_autotune_release است
    Failed    = 5    // شکست خورد؛ دلیلش در mc_autotune_fail است
};

enum class AutotuneFail : uint8_t {
    None         = 0,
    OverTemp     = 1,   // دما از هدف بیش از ۲۰ درجه بالاتر رفت
    Timeout      = 2,   // زمان کل تمام شد
    Sensor       = 3,   // سنسور سالم نیست
    GainsInvalid = 4    // ضرایب نامعتبر یا خارج از بازه
};

constexpr int32_t  kAutotuneMinTargetX10  = 500;    // کمترین هدف مجاز ×۱۰ (50 درجه)
constexpr uint8_t  kAutotuneCyclesDefault = 5;
constexpr uint8_t  kAutotuneCyclesMin     = 3;
constexpr uint8_t  kAutotuneCyclesMax     = 20;

struct AutotuneResult {
    float kp;
    float ki;
    float kd;
};

// شروع تنظیم. target_x10 را M303 قبلاً بررسی کرده است. cycles بین kAutotuneCyclesMin و kAutotuneCyclesMax.
void mc_autotune_begin(int32_t target_x10, uint8_t cycles);

// هر ۱۰۰ میلی‌ثانیه از mc_heater_ex_poll صدا زده می‌شود. توان ۰ تا ۲۵۵ برای هیتر اکسترودر را برمی‌گرداند.
// اگر تنظیم فعال نیست ۰ برمی‌گرداند.
uint8_t mc_autotune_step(bool have_temp, int32_t t_x10);

// آیا تنظیم در جریان است؟ (Heating یا RelayLow یا RelayHigh)
bool mc_autotune_active();
// هدف تنظیم ×۱۰ وقتی فعال است، وگرنه ۰.
int32_t mc_autotune_target_x10();

AutotuneState mc_autotune_state();
AutotuneFail  mc_autotune_fail();
uint8_t mc_autotune_cycle();          // چرخه‌های کامل‌شده
uint8_t mc_autotune_cycles_total();   // تعداد چرخه‌های درخواست‌شده

// آخرین ضرایب محاسبه‌شده (بعد از اولین چرخه کامل). false = هنوز چرخه کامل نشده.
bool mc_autotune_latest(AutotuneResult* out);

// بعد از خواندن نتیجه (Done یا Failed) صدا زده می‌شود و ماشین به Idle برمی‌گردد.
void mc_autotune_release();

}  // namespace mc
