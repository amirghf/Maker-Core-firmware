#pragma once
// کنترل‌کننده PID عمومی (بند ۴.۴). بدون سخت‌افزار. هیچ هدر ST اینجا include نمی‌شود.
// دما بر حسب سلسیوس (float). خروجی توان ۰ تا ۲۵۵ (هم‌مقیاس با output_pwm).
#include <stdint.h>

namespace mc {

struct PidGains {
    float kp;   // تناسبی: توان به ازای هر درجه خطا
    float ki;   // انتگرال: توان به ازای هر (درجه × ثانیه)
    float kd;   // مشتق: توان به ازای هر (درجه بر ثانیه)
};

enum class PidRegion : uint8_t {
    Active = 0,   // PID کار می‌کند
    FullOn = 1,   // خیلی پایین‌تر از هدف: توان کامل
    CutOff = 2    // خیلی بالاتر از هدف: توان صفر
};

constexpr float   kPidOutMax           = 255.0f;   // بیشترین توان
constexpr float   kPidFunctionalRangeC = 15.0f;    // خارج از این فاصله از هدف، PID کار نمی‌کند (توان کامل یا صفر)
constexpr uint8_t kPidDerivSamples     = 10;       // مشتق روی ۱۰ نمونه قبل (۱ ثانیه) حساب می‌شود

class Pid {
public:
    // حافظه داخلی (انتگرال و تاریخچه دما) را پاک می‌کند.
    void reset();
    // یک گام محاسبه. خروجی ۰ تا ۲۵۵. region می‌تواند nullptr باشد.
    float update(const PidGains& g, float target_c, float temp_c, float dt_s, PidRegion* region);
    // مقدار فعلی جمله انتگرال (برای debug)
    float integral() const { return integral_; }
    // آخرین مشتق دما بر حسب درجه بر ثانیه (برای debug)
    float derivative() const { return deriv_; }

private:
    float   integral_ = 0.0f;
    float   deriv_    = 0.0f;
    float   hist_[kPidDerivSamples] = {};
    uint8_t head_  = 0U;
    uint8_t count_ = 0U;
};

}  // namespace mc
