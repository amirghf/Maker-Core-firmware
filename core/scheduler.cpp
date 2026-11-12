// پیاده‌سازی زمان‌بند مشارکتی. هیچ هدر ST اینجا include نمی‌شود.
#include "scheduler.h"
#include "hal_time.h"

namespace mc {

int Scheduler::add(TaskFn fn, uint32_t period_ms) {
    if (fn == nullptr) { return -1; }
    if (count_ >= kMaxTasks) { return -1; }
    Task& t = tasks_[count_];
    t.fn        = fn;
    t.period_ms = period_ms;
    t.last_ms   = mc_millis();
    const int id = static_cast<int>(count_);
    count_ = static_cast<uint8_t>(count_ + 1U);
    return id;
}

void Scheduler::run() {
    for (uint8_t i = 0; i < count_; ++i) {
        Task& t = tasks_[i];
        if ((t.period_ms != 0U) && !mc_time_elapsed_ms(t.last_ms, t.period_ms)) {
            continue;  // هنوز وقتش نشده
        }
        t.last_ms = mc_millis();
        const uint32_t t0 = mc_micros();
        t.fn();
        const uint32_t dt = mc_micros() - t0;
        if (dt > max_us_) { max_us_ = dt; }
    }
}

}  // namespace mc
