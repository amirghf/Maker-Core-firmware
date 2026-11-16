// حدود نرم‌افزاری (بند ۵.۱۰). فقط حالت و عدد نگه می‌دارد و به سخت‌افزار دست نمی‌زند.
#include "soft_limits.h"
#include "machine_state.h"
#include "motion_modes.h"

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند. شماره‌ها دائمی‌اند.
// واحد مقدارهای mm10: دهم میلی‌متر (۲۴۰ میلی‌متر = ۲۴۰۰).
extern "C" {
volatile uint32_t mc_dbg_152_sl_enabled      = 0;  // خواندنی: ۱ = حدها فعال، ۰ = غیرفعال
volatile uint32_t mc_dbg_153_sl_en_set       = 0;  // نوشتنی: ۱ = فعال کن، ۲ = غیرفعال کن. بعد از اجرا خودش صفر می‌شود. مقدار غیرصفر دیگر رد می‌شود.
volatile uint32_t mc_dbg_154_sl_axis         = 0;  // نوشتنی: محور انتخاب‌شده برای بقیه متغیرها (۰ = X ، ۱ = Y ، ۲ = Z). مقدار بیش از ۲ رد می‌شود و محور قبلی می‌ماند.
volatile int32_t  mc_dbg_155_sl_min_mm10     = 0;  // خواندنی: حداقل محور انتخاب‌شده (دهم میلی‌متر)
volatile int32_t  mc_dbg_156_sl_max_mm10     = 0;  // خواندنی: حداکثر محور انتخاب‌شده (دهم میلی‌متر)
volatile int32_t  mc_dbg_157_sl_new_min_mm10 = 0;  // نوشتنی: حداقل جدید (دهم میلی‌متر) که با mc_dbg_159_sl_apply اعمال می‌شود
volatile int32_t  mc_dbg_158_sl_new_max_mm10 = 0;  // نوشتنی: حداکثر جدید (دهم میلی‌متر) که با mc_dbg_159_sl_apply اعمال می‌شود
volatile uint32_t mc_dbg_159_sl_apply        = 0;  // نوشتنی: ۱ = حدهای محور انتخاب‌شده را از mc_dbg_157 و mc_dbg_158 بگذار. خودش صفر می‌شود. مقدار غیرصفر دیگر رد می‌شود.
volatile int32_t  mc_dbg_160_sl_chk_mm10     = 0;  // نوشتنی: مقصد آزمایشی (دهم میلی‌متر) برای محور انتخاب‌شده
volatile uint32_t mc_dbg_161_sl_chk_go       = 0;  // نوشتنی: ۱ = مقصد mc_dbg_160 را برای محور انتخاب‌شده بررسی کن. خودش صفر می‌شود. مقدار غیرصفر دیگر رد می‌شود.
volatile uint32_t mc_dbg_162_sl_chk_result   = 0;  // خواندنی: نتیجه آخرین بررسی. ۰ = هنوز بررسی نشده، ۱ = مجاز، ۲ = رد شد.
volatile uint32_t mc_dbg_163_sl_refused      = 0;  // خواندنی: تعداد کل رد شدن‌ها (نوشتن نامعتبر و مقصد نامجاز)
}

namespace mc {
namespace {

constexpr uint8_t kLimitedAxes = 3U;   // فقط X و Y و Z حد دارند

bool    g_enabled = true;
float   g_min[kLimitedAxes] = { kSoftLimitDefaultMinMm, kSoftLimitDefaultMinMm, kSoftLimitDefaultMinMm };
float   g_max[kLimitedAxes] = { kSoftLimitDefaultMaxXMm, kSoftLimitDefaultMaxYMm, kSoftLimitDefaultMaxZMm };
uint8_t g_sel = 0U;   // محور انتخاب‌شده برای متغیرهای debug

// میلی‌متر ← دهم میلی‌متر (گرد شده، با علامت)
int32_t to_mm10(float v) {
    const float t = v * 10.0f;
    return (t >= 0.0f) ? static_cast<int32_t>(t + 0.5f) : static_cast<int32_t>(t - 0.5f);
}

void refresh_debug() {
    mc_dbg_152_sl_enabled  = g_enabled ? 1U : 0U;
    mc_dbg_154_sl_axis     = g_sel;
    mc_dbg_155_sl_min_mm10 = to_mm10(g_min[g_sel]);
    mc_dbg_156_sl_max_mm10 = to_mm10(g_max[g_sel]);
}

}  // namespace

void mc_softlimit_init() {
    g_enabled = true;
    for (uint8_t i = 0U; i < kLimitedAxes; ++i) {
        g_min[i] = kSoftLimitDefaultMinMm;
    }
    g_max[0] = kSoftLimitDefaultMaxXMm;
    g_max[1] = kSoftLimitDefaultMaxYMm;
    g_max[2] = kSoftLimitDefaultMaxZMm;
    g_sel = 0U;
    mc_dbg_153_sl_en_set       = 0U;
    mc_dbg_157_sl_new_min_mm10 = 0;
    mc_dbg_158_sl_new_max_mm10 = 0;
    mc_dbg_159_sl_apply        = 0U;
    mc_dbg_160_sl_chk_mm10     = 0;
    mc_dbg_161_sl_chk_go       = 0U;
    mc_dbg_162_sl_chk_result   = 0U;
    mc_dbg_163_sl_refused      = 0U;
    refresh_debug();
}

void mc_softlimit_poll() {
    // ۱) انتخاب محور
    const uint32_t axis_req = mc_dbg_154_sl_axis;
    if (axis_req != static_cast<uint32_t>(g_sel)) {
        if (axis_req < static_cast<uint32_t>(kLimitedAxes)) {
            g_sel = static_cast<uint8_t>(axis_req);
        } else {
            mc_softlimit_note_refused();
        }
    }

    // ۲) فعال یا غیرفعال کردن
    const uint32_t en_req = mc_dbg_153_sl_en_set;
    if (en_req != 0U) {
        mc_dbg_153_sl_en_set = 0U;
        if (en_req == 1U) {
            mc_softlimit_enabled_set(true);
        } else if (en_req == 2U) {
            mc_softlimit_enabled_set(false);
        } else {
            mc_softlimit_note_refused();
        }
    }

    // ۳) اعمال حدهای جدید برای محور انتخاب‌شده
    const uint32_t apply_req = mc_dbg_159_sl_apply;
    if (apply_req != 0U) {
        mc_dbg_159_sl_apply = 0U;
        if (apply_req == 1U) {
            const float mn = static_cast<float>(mc_dbg_157_sl_new_min_mm10) / 10.0f;
            const float mx = static_cast<float>(mc_dbg_158_sl_new_max_mm10) / 10.0f;
            if (!mc_softlimit_set(g_sel, mn, mx)) {
                mc_softlimit_note_refused();
            }
        } else {
            mc_softlimit_note_refused();
        }
    }

    // ۴) بررسی یک مقصد آزمایشی
    const uint32_t chk_req = mc_dbg_161_sl_chk_go;
    if (chk_req != 0U) {
        mc_dbg_161_sl_chk_go = 0U;
        if (chk_req == 1U) {
            const float target = static_cast<float>(mc_dbg_160_sl_chk_mm10) / 10.0f;
            if (mc_softlimit_allows(g_sel, target)) {
                mc_dbg_162_sl_chk_result = 1U;
            } else {
                mc_dbg_162_sl_chk_result = 2U;
                mc_softlimit_note_refused();
            }
        } else {
            mc_softlimit_note_refused();
        }
    }

    refresh_debug();
}

bool mc_softlimit_enabled_get() { return g_enabled; }

void mc_softlimit_enabled_set(bool enabled) {
    g_enabled = enabled;
    refresh_debug();
}

float mc_softlimit_min_get(uint8_t axis) {
    if (axis >= kLimitedAxes) { return 0.0f; }
    return g_min[axis];
}

float mc_softlimit_max_get(uint8_t axis) {
    if (axis >= kLimitedAxes) { return 0.0f; }
    return g_max[axis];
}

bool mc_softlimit_set(uint8_t axis, float min_mm, float max_mm) {
    if (axis >= kLimitedAxes) { return false; }
    // NaN هم در این شرط‌ها رد می‌شود
    if (!((min_mm >= -kModeMaxPositionMm) && (min_mm <= kModeMaxPositionMm))) { return false; }
    if (!((max_mm >= -kModeMaxPositionMm) && (max_mm <= kModeMaxPositionMm))) { return false; }
    if (!(min_mm < max_mm)) { return false; }
    g_min[axis] = min_mm;
    g_max[axis] = max_mm;
    refresh_debug();
    return true;
}

bool mc_softlimit_allows(uint8_t axis, float target_mm) {
    if (!g_enabled) { return true; }
    if (axis >= kLimitedAxes) { return true; }   // E (و هر شماره دیگر) حد ندارد
    // NaN در این شرط false می‌شود و رد می‌شود
    return (target_mm >= g_min[axis]) && (target_mm <= g_max[axis]);
}

bool mc_softlimit_allows_all(const float target_mm[4]) {
    for (uint8_t i = 0U; i < kLimitedAxes; ++i) {
        if (!mc_softlimit_allows(i, target_mm[i])) { return false; }
    }
    return true;
}

void mc_softlimit_note_refused() { mc_dbg_163_sl_refused = mc_dbg_163_sl_refused + 1U; }

}  // namespace mc
