#pragma once
// برنامه‌ریز حرکت (بند ۵.۷): پروفایل دوزنقه‌ای هر بلوک، سرعت اتصال بین بلوک‌ها (junction deviation)
// و lookahead. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "../config/mc_config.h"
#include "motion_queue.h"

namespace mc {

// اندازه صف برنامه‌ریز (توان ۲). ظرفیت واقعی یکی کمتر است، یعنی ۱۵ بلوک.
constexpr uint8_t kPlanQueueSize = 16U;
// انحراف اتصال پیش‌فرض (میکرومتر)
constexpr uint32_t kPlanDefaultJunctionUm = MC_JUNCTION_DEVIATION_UM;
constexpr uint32_t kPlanMinJunctionUm     = 1U;
constexpr uint32_t kPlanMaxJunctionUm     = 5000U;

// آنچه وقفه برای اجرای یک بلوک لازم دارد (فقط عدد صحیح).
struct PlanExec {
    uint32_t steps[4];     // گام هر محور
    uint8_t  dir_mask;     // بیت ۰ = X ... بیت ۳ = E؛ ۱ = مثبت
    uint8_t  master;       // شماره محور اصلی
    uint32_t total;        // گام محور اصلی
    uint32_t entry_sq;     // مربع نرخ ورود (گام بر ثانیه، برای محور اصلی همین بلوک)
    uint32_t exit_sq;      // مربع نرخ خروج
    uint32_t nominal_hz;   // نرخ cruise
    uint32_t accel;        // شتاب (گام بر ثانیه‌مربع)
    uint32_t n_acc;        // تعداد گام فاز شتاب‌گیری
    uint32_t n_dec;        // تعداد گام فاز کاهش سرعت
};

// همه چیز را صفر می‌کند. یک‌بار هنگام راه‌اندازی.
void mc_plan_init();
// یک بلوک (ورودی ۵.۶) را وارد صف می‌کند، سرعت ورود و خروج را حساب می‌کند و lookahead را اجرا می‌کند.
// false اگر صف پر است. اعتبارسنجی نرخ و گام با صدا‌زننده (motion_queue) است.
bool mc_plan_push(const MotionBlock& block);
// همه بلوک‌های منتظر را پاک می‌کند. فقط وقتی صدا زده شود که مولد گام متوقف است.
void mc_plan_clear();
// تعداد بلوک‌های منتظر (بدون بلوکی که الان اجرا می‌شود).
uint16_t mc_plan_count();
// فقط از وقفه مولد گام: اولین بلوک منتظر را برمی‌دارد. false اگر صف خالی است.
bool mc_plan_isr_take(PlanExec& out);
// وظیفه زمان‌بند (هر ۱۰ میلی‌ثانیه): اگر برنامه‌ریزی دوباره لازم است انجامش می‌دهد و متغیرهای debug را به‌روز می‌کند.
void mc_plan_poll();
// تنظیم و دریافت انحراف اتصال (junction deviation) به میکرومتر (بند ۵.۱۱)
bool mc_plan_jd_set(uint32_t um);
uint32_t mc_plan_jd_get();

}  // namespace mc
