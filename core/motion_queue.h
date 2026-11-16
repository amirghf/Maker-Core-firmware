#pragma once
// صف بلوک‌های حرکت (بند ۵.۶ و ۵.۷). هیچ هدر ST اینجا include نمی‌شود.
// از بند ۵.۷ بلوک‌ها در planner نگه‌داری می‌شوند و مولد گام آن‌ها را بدون توقف اجرا می‌کند.
#include <stdint.h>

namespace mc {

// یک بلوک حرکت: یک حرکت دوزنقه‌ای چندمحوره (همان ورودی‌های mc_axis_move_start_multi)
struct MotionBlock {
    uint32_t steps[4];     // گام هر محور: X، Y، Z، E
    uint32_t start_hz;     // نرخ شروع و پایان (گام بر ثانیه)
    uint32_t cruise_hz;    // نرخ cruise (گام بر ثانیه)
    uint32_t accel_sps2;   // شتاب (گام بر ثانیه‌مربع)
    uint8_t  dir_mask;     // بیت ۰ = X ... بیت ۳ = E؛ ۱ = مثبت
};

// دلیل رد شدن آخرین بلوک ورودی
enum class MqReason : uint8_t {
    None         = 0,
    Full         = 1,   // صف پر است (۱۵ بلوک، kPlanQueueSize در planner.h)
    ZeroSteps    = 2,   // گام همه محورها صفر است
    TooManySteps = 3,   // گام یک محور بیشتر از kAxisMoveMaxSteps است
    BadRate      = 4,   // نرخ شروع یا cruise یا شتاب بیرون از بازه است
    Killed       = 5,   // برد kill شده
    NotArmed     = 6,   // mc_dbg_80_mv_arm برابر 1 نیست (فقط مسیر تست)
    ColdExtrude  = 7     // اکستروژن سرد یا سنسور دمای اکسترودر سالم نیست (بند ۵.۹)
};

// وضعیت صف
enum class MqState : uint8_t {
    Idle    = 0,   // صف خالی و هیچ بلوکی در حال اجرا نیست
    Waiting = 1,   // بلوک در صف هست ولی اجرا مجاز نیست (mc_dbg_112_mq_run برابر 1 نیست)
    Running = 2,   // یک بلوک در حال حرکت است یا صف در حال اجراست
    Cleared = 3    // صف به‌خاطر abort، kill، arm، شروع ناموفق یا دستور clear پاک شد (تا ورود بلوک بعدی می‌ماند)
};

// همه چیز را صفر می‌کند. یک‌بار هنگام راه‌اندازی.
void mc_mq_init();
// یک بلوک را بعد از بررسی به صف اضافه می‌کند. false اگر رد شد (دلیل در متغیر debug).
bool mc_mq_push(const MotionBlock& block);
// همه چیز را پاک می‌کند: بلوک‌های منتظر و (بند ۵.۷) اجرای جاری را هم قطع می‌کند.
void mc_mq_clear();
// تعداد بلوک‌های منتظر در صف (بدون بلوک در حال اجرا).
uint16_t mc_mq_count();
// آیا بلوکی در صف هست یا بلوکی در حال اجراست؟
bool mc_mq_busy();
// وظیفه زمان‌بند (هر ۱۰ میلی‌ثانیه): اجرای بلوک‌ها، مسیر تست با متغیرهای debug و به‌روزرسانی آن‌ها.
void mc_mq_poll();

}  // namespace mc
