// پارامترهای ترمیستور برد MW-X1 (بند ۴.۲).
// این فایل هدر ST ندارد و فقط عددهای برد را می‌دهد.
#include "hal_thermistor.h"

namespace mc {
namespace {

// ترتیب دقیقاً مثل enum class AdcChannel (اکسترودر، بستر).
// مقدارهای موقت: پول‌آپ ۶.۵ کیلواهم (با مولتی‌متر اندازه‌گیری شد، احتمالاً ۶.۸K اسمی)،
// ترمیستور NTC 100K در ۲۵ درجه، بتا ۳۹۵۰ (حدس رایج؛ سازنده معلوم نیست).
// در بند ۴.۴ با دماسنج مرجع اصلاح می‌شوند.
constexpr ThermistorParams kParams[] = {
    { 6500.0f, 100000.0f, 25.0f, 3950.0f },   // اکسترودر
    { 6500.0f, 100000.0f, 25.0f, 3950.0f }    // بستر
};

}  // namespace

bool mc_thermistor_params(AdcChannel ch, ThermistorParams* out) {
    if (out == nullptr) { return false; }
    const unsigned idx = static_cast<unsigned>(ch);
    if (idx >= static_cast<unsigned>(AdcChannel::Count)) { return false; }
    *out = kParams[idx];
    return true;
}

}  // namespace mc
