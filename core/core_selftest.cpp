// سلف‌تست‌های هسته MakerCore. هیچ هدر ST اینجا include نمی‌شود.
#include "core_selftest.h"
#include "ring_buffer.h"
#include "scheduler.h"
#include "hal_time.h"

// متغیر دیباگ؛ در CubeIDE با Live Expressions دیده می‌شود.
extern "C" {
volatile int      mc_ring_selftest_ok    = 0;  // 1 = سلف‌تست بافر حلقوی پاس شد
volatile int      mc_sched_selftest_ok   = 0;  // 1 = سلف‌تست زمان‌بند پاس شد
volatile uint32_t mc_dbg_sched_test_fast = 0;  // تعداد اجرای وظیفه ۱۰ میلی‌ثانیه‌ای در تست
}

namespace mc {
namespace {

bool ring_selftest() {
    RingBuffer<uint8_t, 8> rb;  // ظرفیت واقعی ۷

    // ۱) در شروع خالی است
    uint8_t v = 0;
    if (!rb.empty() || rb.size() != 0U || rb.full()) { return false; }
    if (rb.pop(v)) { return false; }
    if (rb.capacity() != 7U) { return false; }

    // ۲) ۷ عنصر جا می‌شود و هشتمی رد می‌شود
    for (uint8_t i = 0; i < 7U; ++i) {
        if (!rb.push(static_cast<uint8_t>(i + 10U))) { return false; }
    }
    if (!rb.full() || rb.size() != 7U) { return false; }
    if (rb.push(99U)) { return false; }

    // ۳) peek عنصر اول را بدون برداشتن نشان می‌دهد
    if (!rb.peek(v) || v != 10U || rb.size() != 7U) { return false; }

    // ۴) ترتیب خروج درست است (اولین ورودی، اولین خروجی)
    for (uint8_t i = 0; i < 7U; ++i) {
        if (!rb.pop(v) || v != static_cast<uint8_t>(i + 10U)) { return false; }
    }
    if (!rb.empty()) { return false; }

    // ۵) چرخش اندیس‌ها: ۳۰ دور نوشتن و خواندن (بیشتر از اندازه بافر)
    for (uint8_t i = 0; i < 30U; ++i) {
        if (!rb.push(i)) { return false; }
        if (!rb.push(static_cast<uint8_t>(i + 100U))) { return false; }
        if (!rb.pop(v) || v != i) { return false; }
        if (!rb.pop(v) || v != static_cast<uint8_t>(i + 100U)) { return false; }
    }
    if (!rb.empty()) { return false; }

    // ۶) clear بافر را خالی می‌کند
    (void)rb.push(1U);
    (void)rb.push(2U);
    rb.clear();
    if (!rb.empty() || rb.size() != 0U) { return false; }

    return true;
}

uint32_t g_cnt_fast  = 0;  // شمارنده وظیفه ۱۰ میلی‌ثانیه‌ای
uint32_t g_cnt_every = 0;  // شمارنده وظیفه‌ای که هر دور اجرا می‌شود

void task_fast()  { g_cnt_fast  = g_cnt_fast  + 1U; }
void task_every() { g_cnt_every = g_cnt_every + 1U; }
void task_dummy() {}

bool sched_selftest() {
    Scheduler s;
    g_cnt_fast  = 0;
    g_cnt_every = 0;

    // ۱) تابع خالی پذیرفته نمی‌شود
    if (s.add(nullptr, 10U) != -1) { return false; }
    // ۲) شماره وظیفه‌ها از صفر و پشت‌سرهم است
    if (s.add(task_fast, 10U) != 0) { return false; }
    if (s.add(task_every, 0U) != 1) { return false; }
    // ۳) جدول تا ظرفیت پر می‌شود و بعدش رد می‌کند
    for (uint8_t i = 2; i < Scheduler::kMaxTasks; ++i) {
        if (s.add(task_dummy, 1000U) != static_cast<int>(i)) { return false; }
    }
    if (s.add(task_dummy, 1000U) != -1) { return false; }
    if (s.count() != Scheduler::kMaxTasks) { return false; }

    // ۴) ۱۰۰ میلی‌ثانیه حلقه را می‌چرخانیم
    const uint32_t start = mc_millis();
    while (!mc_time_elapsed_ms(start, 100U)) {
        s.run();
    }
    mc_dbg_sched_test_fast = g_cnt_fast;
    // وظیفه ۱۰ میلی‌ثانیه‌ای باید حدود ۱۰ بار اجرا شده باشد
    if ((g_cnt_fast < 8U) || (g_cnt_fast > 11U)) { return false; }
    // وظیفه هر دور باید خیلی بیشتر اجرا شده باشد
    if (g_cnt_every < 20U) { return false; }
    return true;
}

}  // namespace
}  // namespace mc

extern "C" int mc_ring_selftest_c(void) {
    const bool ok = mc::ring_selftest();
    mc_ring_selftest_ok = ok ? 1 : 0;
    return ok ? 1 : 0;
}

extern "C" int mc_sched_selftest_c(void) {
    const bool ok = mc::sched_selftest();
    mc_sched_selftest_ok = ok ? 1 : 0;
    return ok ? 1 : 0;
}
