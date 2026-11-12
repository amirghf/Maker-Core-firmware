#include "adc_sampler.h"
#include "hal_time.h"

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_adc_ext_avg16 = 0;   // میانگین ×۱۶ اکسترودر (۰ = هنوز معتبر نیست)
volatile uint32_t mc_dbg_adc_bed_avg16 = 0;   // میانگین ×۱۶ بستر (۰ = هنوز معتبر نیست)
volatile uint32_t mc_dbg_adc_ext_noise = 0;   // بیشترین منهای کمترین نمونه خام در پنجره اکسترودر
volatile uint32_t mc_dbg_adc_bed_noise = 0;   // بیشترین منهای کمترین نمونه خام در پنجره بستر
volatile uint32_t mc_dbg_adc_samples   = 0;   // تعداد دورهای نمونه‌گیری از شروع
volatile uint32_t mc_dbg_adc_fail      = 0;   // تعداد خواندن‌های ناموفق ADC (جمع دو کانال)
volatile uint32_t mc_dbg_adc_ready     = 0;   // ۱ = هر دو کانال معتبرند
volatile uint32_t mc_dbg_adc_task_us   = 0;   // بیشترین زمان اجرای یک دور نمونه‌گیری (میکروثانیه)
volatile uint32_t mc_dbg_adc_fake_fail = 0;   // برای تست: نوشتن ۱ یعنی «خواندن ADC شکست خورد»، ۰ یعنی عادی
}

namespace mc {
namespace {

constexpr uint8_t kChannelCount = static_cast<uint8_t>(AdcChannel::Count);

struct ChannelState {
    uint16_t win[kAdcWindow];   // آخرین نمونه‌های خام
    uint8_t  head;              // جای نمونه بعدی در پنجره
    uint8_t  filled;            // تعداد نمونه‌های پنجره (حداکثر kAdcWindow)
    uint8_t  fail_run;          // خطاهای پشت‌سرهم
    bool     valid;             // میانگین قابل‌اعتماد است
    bool     has_sample;        // حداقل یک نمونه سالم داشته‌ایم
    uint32_t sum;               // جمع نمونه‌های پنجره
    uint32_t last_ok_ms;        // زمان آخرین نمونه سالم
};

ChannelState g_ch[kChannelCount] = {};

void reset_window(ChannelState& s) {
    for (uint8_t i = 0U; i < kAdcWindow; ++i) { s.win[i] = 0U; }
    s.head   = 0U;
    s.filled = 0U;
    s.sum    = 0U;
    s.valid  = false;
}

void push_sample(ChannelState& s, uint16_t raw) {
    if (s.filled == kAdcWindow) {
        s.sum -= s.win[s.head];            // قدیمی‌ترین نمونه بیرون می‌رود
    } else {
        s.filled = static_cast<uint8_t>(s.filled + 1U);
    }
    s.win[s.head] = raw;
    s.sum += raw;
    s.head = static_cast<uint8_t>(s.head + 1U);
    if (s.head >= kAdcWindow) { s.head = 0U; }
    s.fail_run   = 0U;
    s.last_ok_ms = mc_millis();
    s.has_sample = true;
    if (s.filled == kAdcWindow) { s.valid = true; }
}

void note_failure(ChannelState& s) {
    mc_dbg_adc_fail = mc_dbg_adc_fail + 1U;
    if (s.fail_run < 255U) { s.fail_run = static_cast<uint8_t>(s.fail_run + 1U); }
    if (s.fail_run >= kAdcMaxFail) { reset_window(s); }   // نمونه‌های قدیمی دور ریخته می‌شوند
}

uint32_t window_noise(const ChannelState& s) {
    if (s.filled == 0U) { return 0U; }
    uint16_t lo = 0xFFFFU;
    uint16_t hi = 0U;
    for (uint8_t i = 0U; i < s.filled; ++i) {
        const uint16_t v = s.win[i];
        if (v < lo) { lo = v; }
        if (v > hi) { hi = v; }
    }
    return static_cast<uint32_t>(hi - lo);
}

}  // namespace

void mc_adc_sampler_poll() {
    const uint32_t t0 = mc_micros();

    for (uint8_t i = 0U; i < kChannelCount; ++i) {
        ChannelState& s = g_ch[i];
        uint16_t raw = 0U;
        bool ok = false;
        if (mc_dbg_adc_fake_fail == 0U) {
            ok = mc_adc_read(static_cast<AdcChannel>(i), &raw);
        }
        if (ok && (raw <= 4095U)) {
            push_sample(s, raw);
        } else {
            note_failure(s);
        }
    }

    const ChannelState& ext = g_ch[static_cast<uint8_t>(AdcChannel::TempExtruder)];
    const ChannelState& bed = g_ch[static_cast<uint8_t>(AdcChannel::TempBed)];
    mc_dbg_adc_ext_avg16 = ext.valid ? ext.sum : 0U;
    mc_dbg_adc_bed_avg16 = bed.valid ? bed.sum : 0U;
    mc_dbg_adc_ext_noise = window_noise(ext);
    mc_dbg_adc_bed_noise = window_noise(bed);
    mc_dbg_adc_ready     = (ext.valid && bed.valid) ? 1U : 0U;
    mc_dbg_adc_samples   = mc_dbg_adc_samples + 1U;

    const uint32_t used = mc_micros() - t0;
    if (used > mc_dbg_adc_task_us) { mc_dbg_adc_task_us = used; }
}

bool mc_adc_sampler_valid(AdcChannel ch) {
    const unsigned idx = static_cast<unsigned>(ch);
    if (idx >= kChannelCount) { return false; }
    return g_ch[idx].valid;
}

bool mc_adc_sampler_ready() {
    for (uint8_t i = 0U; i < kChannelCount; ++i) {
        if (!g_ch[i].valid) { return false; }
    }
    return true;
}

bool mc_adc_avg_x16(AdcChannel ch, uint16_t* avg_x16) {
    if (avg_x16 == nullptr) { return false; }
    const unsigned idx = static_cast<unsigned>(ch);
    if (idx >= kChannelCount) { return false; }
    if (!g_ch[idx].valid) { return false; }
    *avg_x16 = static_cast<uint16_t>(g_ch[idx].sum);
    return true;
}

uint32_t mc_adc_sample_age_ms(AdcChannel ch) {
    const unsigned idx = static_cast<unsigned>(ch);
    if (idx >= kChannelCount) { return 0xFFFFFFFFU; }
    if (!g_ch[idx].has_sample) { return 0xFFFFFFFFU; }
    return mc_millis() - g_ch[idx].last_ok_ms;
}

}  // namespace mc
