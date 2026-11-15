#pragma once
// حرکت یک محور با سرعت ثابت یا دوزنقه‌ای (بند ۵.۳ و ۵.۴). هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "hal_stepper.h"

namespace mc {

// سقف ایمنی هر حرکت (گام). برد به دستگاه وصل است و طول مسیر محورها هنوز معلوم نیست، پس کوچک گذاشته شده.
constexpr uint32_t kAxisMoveMaxSteps  = 2000U;
// سقف نرخ هر حرکت (گام در ثانیه).
constexpr uint32_t kAxisMoveMaxRateHz = 2000U;
// بازه مجاز شتاب حرکت دوزنقه‌ای (گام بر ثانیه‌مربع) — بند ۵.۴
constexpr uint32_t kAxisAccelMinSps2 = 50U;
constexpr uint32_t kAxisAccelMaxSps2 = 20000U;

// وضعیت لایه حرکت
enum class MoveState : uint8_t {
    Idle    = 0,   // هنوز حرکتی نشده
    Moving  = 1,   // در حال حرکت
    Done    = 2,   // حرکت کامل تمام شد
    Aborted = 3,   // حرکت وسط کار قطع شد
    Refused = 4    // آخرین درخواست رد شد
};

// دلیل رد شدن آخرین درخواست
enum class MoveRefuse : uint8_t {
    None         = 0,
    Killed       = 1,   // برد kill شده
    BadAxis      = 2,   // شماره محور نامعتبر
    ZeroSteps    = 3,   // تعداد گام صفر
    TooManySteps = 4,   // گام بیشتر از kAxisMoveMaxSteps
    BadRate      = 5,   // نرخ بیرون از بازه
    Busy         = 6,   // حرکت دیگری در جریان است
    NotArmed     = 7,   // mc_dbg_80_mv_arm برابر 1 نیست (فقط مسیر تست)
    StartFailed  = 8,   // مولد گام شروع نکرد
    BadAccel     = 9    // شتاب بیرون از بازه (بند ۵.۴)
};

// همه چیز را صفر می‌کند. یک‌بار هنگام راه‌اندازی.
void mc_axis_move_init();
// یک حرکت با سرعت ثابت را شروع می‌کند. false اگر رد شد (دلیل در متغیر debug ثبت می‌شود).
bool mc_axis_move_start(StepAxis axis, bool forward, uint32_t steps, uint32_t rate_hz);
// یک حرکت دوزنقه‌ای را شروع می‌کند (بند ۵.۴). false اگر رد شد (دلیل در متغیر debug ثبت می‌شود).
bool mc_axis_move_start_ramp(StepAxis axis, bool forward, uint32_t steps,
                             uint32_t start_hz, uint32_t cruise_hz, uint32_t accel_sps2);
// حرکت جاری را قطع می‌کند (بی‌اثر اگر حرکتی در جریان نیست).
void mc_axis_move_abort();
// آیا حرکتی در جریان است؟
bool mc_axis_move_busy();
// موقعیت محور بر حسب گام (از روشن‌شدن برد). محور نامعتبر = ۰.
int32_t mc_axis_position_steps(StepAxis axis);
// وضعیت فعلی.
MoveState mc_axis_move_state();
// وظیفه زمان‌بند (هر ۱۰ میلی‌ثانیه): تشخیص پایان حرکت، مسیر تست با متغیرهای debug، و به‌روزرسانی آن‌ها.
void mc_axis_move_poll();

}  // namespace mc
