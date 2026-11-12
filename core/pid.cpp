// کنترل‌کننده PID عمومی (بند ۴.۴). بدون سخت‌افزار.
#include "pid.h"

namespace mc {

void Pid::reset() {
    integral_ = 0.0f;
    deriv_    = 0.0f;
    head_     = 0U;
    count_    = 0U;
    for (uint8_t i = 0U; i < kPidDerivSamples; ++i) {
        hist_[i] = 0.0f;
    }
}

float Pid::update(const PidGains& g, float target_c, float temp_c, float dt_s, PidRegion* region) {
    if (dt_s <= 0.0f) { dt_s = 0.1f; }

    // ۱) مشتق دما روی ۱ ثانیه (برای کم‌کردن نویز). تا وقتی تاریخچه پر نشده، مشتق صفر است.
    deriv_ = 0.0f;
    if (count_ >= kPidDerivSamples) {
        deriv_ = (temp_c - hist_[head_]) / (static_cast<float>(kPidDerivSamples) * dt_s);
    }
    hist_[head_] = temp_c;
    head_ = static_cast<uint8_t>((head_ + 1U) % kPidDerivSamples);
    if (count_ < kPidDerivSamples) { count_ = static_cast<uint8_t>(count_ + 1U); }

    const float err = target_c - temp_c;

    // ۲) خیلی پایین یا خیلی بالای هدف: PID کار نمی‌کند و انتگرال صفر می‌شود.
    if (err > kPidFunctionalRangeC) {
        integral_ = 0.0f;
        if (region != nullptr) { *region = PidRegion::FullOn; }
        return kPidOutMax;
    }
    if (err < -kPidFunctionalRangeC) {
        integral_ = 0.0f;
        if (region != nullptr) { *region = PidRegion::CutOff; }
        return 0.0f;
    }
    if (region != nullptr) { *region = PidRegion::Active; }

    // ۳) PID. انتگرال بین ۰ و ۲۵۵ نگه داشته می‌شود و وقتی خروجی از بالا اشباع شده و خطا هنوز مثبت است زیاد نمی‌شود.
    const float p_term = g.kp * err;
    const float d_term = -g.kd * deriv_;   // مشتق روی دما (نه روی خطا)، تا با عوض‌شدن هدف جهش نکند

    float i_new = integral_ + g.ki * err * dt_s;
    if (i_new < 0.0f)        { i_new = 0.0f; }
    if (i_new > kPidOutMax)  { i_new = kPidOutMax; }
    const float out_pre = p_term + i_new + d_term;
    if (!((out_pre > kPidOutMax) && (err > 0.0f))) {
        integral_ = i_new;
    }

    float out = p_term + integral_ + d_term;
    if (out < 0.0f)       { out = 0.0f; }
    if (out > kPidOutMax) { out = kPidOutMax; }
    return out;
}

}  // namespace mc
