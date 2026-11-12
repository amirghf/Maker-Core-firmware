#pragma once
// بافر حلقوی ثابت‌اندازه، بدون حافظه پویا.
// فقط برای یک نویسنده و یک خواننده امن است (مثلاً وقفه می‌نویسد و حلقه اصلی می‌خواند).
// ظرفیت واقعی = N منهای ۱ (یک خانه برای تشخیص پر و خالی خالی می‌ماند).
#include <stdint.h>

namespace mc {

template <typename T, uint16_t N>
class RingBuffer {
    static_assert(N >= 2U, "N باید حداقل ۲ باشد");
    static_assert((N & (N - 1U)) == 0U, "N باید توان ۲ باشد");

public:
    // یک عنصر اضافه می‌کند. false = بافر پر است (عنصر اضافه نشد).
    bool push(const T& value) {
        const uint16_t h    = head_;
        const uint16_t next = static_cast<uint16_t>((h + 1U) & (N - 1U));
        if (next == tail_) { return false; }
        buf_[h] = value;
        __atomic_thread_fence(__ATOMIC_RELEASE);  // اول داده نوشته شود، بعد اندیس عوض شود
        head_ = next;
        return true;
    }

    // یک عنصر برمی‌دارد. false = بافر خالی است.
    bool pop(T& out) {
        const uint16_t t = tail_;
        if (t == head_) { return false; }
        __atomic_thread_fence(__ATOMIC_ACQUIRE);
        out = buf_[t];
        __atomic_thread_fence(__ATOMIC_RELEASE);
        tail_ = static_cast<uint16_t>((t + 1U) & (N - 1U));
        return true;
    }

    // عنصر بعدی را بدون برداشتن نشان می‌دهد. false = خالی.
    bool peek(T& out) const {
        const uint16_t t = tail_;
        if (t == head_) { return false; }
        __atomic_thread_fence(__ATOMIC_ACQUIRE);
        out = buf_[t];
        return true;
    }

    // فقط از سمت خواننده (حلقه اصلی) صدا زده شود.
    void clear() { tail_ = head_; }

    uint16_t size() const {
        return static_cast<uint16_t>((head_ - tail_) & (N - 1U));
    }
    uint16_t capacity() const { return static_cast<uint16_t>(N - 1U); }
    bool empty() const { return head_ == tail_; }
    bool full() const { return static_cast<uint16_t>((head_ + 1U) & (N - 1U)) == tail_; }

private:
    T buf_[N] = {};
    volatile uint16_t head_ = 0;  // جای نوشتن بعدی
    volatile uint16_t tail_ = 0;  // جای خواندن بعدی
};

}  // namespace mc
