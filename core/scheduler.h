#pragma once
// زمان‌بند مشارکتی: هر وظیفه یک تابع بدون ورودی است که با دوره مشخص صدا زده می‌شود.
// حافظه ثابت است و حافظه پویا استفاده نمی‌شود. وظیفه‌ها باید کوتاه باشند و منتظر نمانند.
#include <stdint.h>

namespace mc {

class Scheduler {
public:
    static constexpr uint8_t kMaxTasks = 16;
    using TaskFn = void (*)();

    // وظیفه جدید ثبت می‌کند. period_ms = 0 یعنی هر دور حلقه اجرا شود.
    // برمی‌گرداند: شماره وظیفه (از ۰) ، یا -1 اگر تابع خالی باشد یا جا نباشد.
    int add(TaskFn fn, uint32_t period_ms);
    // هر دور حلقه اصلی یک‌بار صدا زده شود.
    void run();
    // تعداد وظیفه‌های ثبت‌شده.
    uint8_t count() const { return count_; }
    // طولانی‌ترین زمان اجرای یک وظیفه (میکروثانیه) از شروع.
    uint32_t max_task_us() const { return max_us_; }

private:
    struct Task {
        TaskFn   fn;
        uint32_t period_ms;
        uint32_t last_ms;
    };
    Task     tasks_[kMaxTasks] = {};
    uint8_t  count_  = 0;
    uint32_t max_us_ = 0;
};

}  // namespace mc
