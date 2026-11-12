#include "thermistor.h"
#include "adc_sampler.h"
#include <string.h>

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile int32_t  mc_dbg_temp_ext_x10    = 0;   // دمای اکسترودر ×۱۰ (250 = 25.0 درجه)؛ ۰ وقتی وضعیت Ok نیست
volatile int32_t  mc_dbg_temp_bed_x10    = 0;   // دمای بستر ×۱۰؛ ۰ وقتی وضعیت Ok نیست
volatile uint32_t mc_dbg_temp_ext_status = 0;   // وضعیت اکسترودر: 0=NotReady 1=Ok 2=Open 3=Short
volatile uint32_t mc_dbg_temp_bed_status = 0;   // وضعیت بستر: 0=NotReady 1=Ok 2=Open 3=Short
volatile uint32_t mc_dbg_temp_ext_ohm    = 0;   // مقاومت محاسبه‌شده ترمیستور اکسترودر (اهم)؛ ۰ وقتی Ok نیست
volatile uint32_t mc_dbg_temp_bed_ohm    = 0;   // مقاومت محاسبه‌شده ترمیستور بستر (اهم)؛ ۰ وقتی Ok نیست
volatile uint32_t mc_dbg_temp_polls      = 0;   // تعداد دورهای تبدیل از شروع
volatile uint32_t mc_dbg_temp_fake_ext   = 0;   // برای تست: عدد غیرصفر = میانگین ×۱۶ ساختگی برای اکسترودر (۰ = عادی)
volatile uint32_t mc_dbg_temp_fake_bed   = 0;   // برای تست: عدد غیرصفر = میانگین ×۱۶ ساختگی برای بستر (۰ = عادی)
}

namespace mc {
namespace {

constexpr uint8_t kChannelCount = static_cast<uint8_t>(AdcChannel::Count);

TempStatus g_status[kChannelCount] = {};
int32_t    g_temp_x10[kChannelCount] = {};

// لگاریتم طبیعی بدون کتابخانه math (x باید بزرگ‌تر از صفر باشد).
// x = m × 2^e با m بین 0.707 و 1.414، بعد ln(m) با سری 2×atanh(z) محاسبه می‌شود.
float mc_ln_f(float x) {
    uint32_t bits = 0U;
    memcpy(&bits, &x, sizeof(bits));
    int32_t e = static_cast<int32_t>((bits >> 23) & 0xFFU) - 127;
    bits = (bits & 0x007FFFFFU) | 0x3F800000U;   // m بین ۱ و ۲
    float m = 1.0f;
    memcpy(&m, &bits, sizeof(m));
    if (m > 1.41421356f) {
        m = m * 0.5f;
        e = e + 1;
    }
    const float z  = (m - 1.0f) / (m + 1.0f);
    const float z2 = z * z;
    const float s  = z * (1.0f + z2 * (0.33333333f + z2 * (0.2f + z2 * (0.14285714f + z2 * 0.11111111f))));
    return 2.0f * s + static_cast<float>(e) * 0.69314718f;
}

void store_result(uint8_t idx, TempStatus st, int32_t t_x10, uint32_t ohm) {
    const bool ok = (st == TempStatus::Ok);
    const int32_t  t_out   = ok ? t_x10 : 0;
    const uint32_t ohm_out = ok ? ohm : 0U;
    g_status[idx]   = st;
    g_temp_x10[idx] = t_out;
    if (idx == static_cast<uint8_t>(AdcChannel::TempExtruder)) {
        mc_dbg_temp_ext_status = static_cast<uint32_t>(st);
        mc_dbg_temp_ext_x10    = t_out;
        mc_dbg_temp_ext_ohm    = ohm_out;
    } else {
        mc_dbg_temp_bed_status = static_cast<uint32_t>(st);
        mc_dbg_temp_bed_x10    = t_out;
        mc_dbg_temp_bed_ohm    = ohm_out;
    }
}

}  // namespace

TempStatus mc_temp_convert(const ThermistorParams& p, uint16_t avg_x16, int32_t* temp_x10, uint32_t* ohm) {
    if ((p.pullup_ohm <= 0.0f) || (p.r0_ohm <= 0.0f) || (p.beta <= 0.0f)) { return TempStatus::NotReady; }
    if (avg_x16 >= kAdcOpenX16)  { return TempStatus::Open; }
    if (avg_x16 <= kAdcShortX16) { return TempStatus::Short; }

    const float a   = static_cast<float>(avg_x16);
    const float rth = p.pullup_ohm * a / (static_cast<float>(kAdcFullX16) - a);   // مقاومت ترمیستور
    const float inv_t = 1.0f / (p.t0_c + 273.15f) + mc_ln_f(rth / p.r0_ohm) / p.beta;
    if (inv_t <= 0.0f) { return TempStatus::Short; }
    const float t_c = 1.0f / inv_t - 273.15f;

    float v = t_c * 10.0f;
    v = (v >= 0.0f) ? (v + 0.5f) : (v - 0.5f);    // گرد کردن
    int32_t t10 = static_cast<int32_t>(v);
    if (t10 < kTempMinX10) { t10 = kTempMinX10; }
    if (t10 > kTempMaxX10) { t10 = kTempMaxX10; }

    if (temp_x10 != nullptr) { *temp_x10 = t10; }
    if (ohm != nullptr)      { *ohm = static_cast<uint32_t>(rth + 0.5f); }
    return TempStatus::Ok;
}

void mc_temp_poll() {
    for (uint8_t i = 0U; i < kChannelCount; ++i) {
        const AdcChannel ch = static_cast<AdcChannel>(i);
        ThermistorParams p = {};
        if (!mc_thermistor_params(ch, &p)) {
            store_result(i, TempStatus::NotReady, 0, 0U);
            continue;
        }
        const uint32_t fake = (ch == AdcChannel::TempExtruder) ? mc_dbg_temp_fake_ext : mc_dbg_temp_fake_bed;
        uint16_t avg = 0U;
        bool have = false;
        if (fake != 0U) {
            avg  = static_cast<uint16_t>((fake > 65535U) ? 65535U : fake);
            have = true;
        } else {
            have = mc_adc_avg_x16(ch, &avg);
        }
        if (!have) {
            store_result(i, TempStatus::NotReady, 0, 0U);
            continue;
        }
        int32_t t10 = 0;
        uint32_t ohm = 0U;
        const TempStatus st = mc_temp_convert(p, avg, &t10, &ohm);
        store_result(i, st, t10, ohm);
    }
    mc_dbg_temp_polls = mc_dbg_temp_polls + 1U;
}

TempStatus mc_temp_status(AdcChannel ch) {
    const unsigned idx = static_cast<unsigned>(ch);
    if (idx >= kChannelCount) { return TempStatus::NotReady; }
    return g_status[idx];
}

bool mc_temp_get_x10(AdcChannel ch, int32_t* temp_x10) {
    if (temp_x10 == nullptr) { return false; }
    const unsigned idx = static_cast<unsigned>(ch);
    if (idx >= kChannelCount) { return false; }
    if (g_status[idx] != TempStatus::Ok) { return false; }
    *temp_x10 = g_temp_x10[idx];
    return true;
}

}  // namespace mc
