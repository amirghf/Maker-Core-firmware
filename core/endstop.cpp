// خواندن اندستاپ‌ها (بند ۶.۱). به سخت‌افزار فقط از راه hal_gpio.h دست می‌زند.
#include "endstop.h"
#include "hal_gpio.h"
#include "hal_time.h"
#include "log.h"

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند. شماره‌ها دائمی‌اند.
// ماسک‌ها: بیت ۰ = X ، بیت ۱ = Y ، بیت ۲ = Z.
extern "C" {
volatile uint32_t mc_dbg_189_es_raw_mask       = 0;  // خواندنی: سطح خام پین‌ها (۱ = HIGH). فقط پین واقعی، بدون اندستاپ ساختگی.
volatile uint32_t mc_dbg_190_es_stable_mask    = 0;  // خواندنی: وضعیت پایدار بعد از debounce (۱ = زده شده). اندستاپ ساختگی هم اثر دارد.
volatile uint32_t mc_dbg_191_es_hits_x         = 0;  // خواندنی: تعداد زده شدن X (آزاد به فشرده، بعد از debounce)
volatile uint32_t mc_dbg_192_es_hits_y         = 0;  // خواندنی: تعداد زده شدن Y
volatile uint32_t mc_dbg_193_es_hits_z         = 0;  // خواندنی: تعداد زده شدن Z
volatile uint32_t mc_dbg_194_es_glitches       = 0;  // خواندنی: تعداد تغییرهای رد‌شده چون سطح قبل از پایان debounce برگشت
volatile uint32_t mc_dbg_195_es_fake_mask      = 0;  // نوشتنی: بیت‌های ۰ تا ۲ (مقدار ۰ تا ۷) سطح نمونه آن محور را HIGH فرض می‌کنند (اندستاپ ساختگی). ۰ = خاموش. فقط سه بیت پایین اثر دارد.
volatile uint32_t mc_dbg_196_es_fake_pulse_ms  = 0;  // نوشتنی: مقدار ۱ تا ۱۰۰۰ = یک پالس ساختگی به این مدت (میلی‌ثانیه) روی X. بعد از پایان پالس خودش صفر می‌شود. مقدار بیرون از ۱ تا ۱۰۰۰ نادیده گرفته و صفر می‌شود.
volatile uint32_t mc_dbg_197_es_poll_runs      = 0;  // خواندنی: تعداد اجرای وظیفه خواندن اندستاپ
volatile uint32_t mc_dbg_198_es_gap_max_ms     = 0;  // خواندنی: بیشترین فاصله بین دو اجرای پشت‌سرهم وظیفه (میلی‌ثانیه)
}

namespace mc {
namespace {

// ترتیب باید X و Y و Z باشد (مثل kAxisX و kAxisY و kAxisZ).
constexpr PinId kPins[kEndstopAxes] = { PinId::LimitXMin, PinId::LimitYMin, PinId::LimitZMin };

constexpr uint32_t kPulseMaxMs = 1000U;   // بیشترین مدت پالس ساختگی

bool     g_stable[kEndstopAxes]    = { false, false, false };   // وضعیت پایدار
bool     g_cand[kEndstopAxes]      = { false, false, false };   // آخرین سطح نمونه‌گیری‌شده (کاندیدای تغییر)
uint32_t g_since_ms[kEndstopAxes]  = { 0U, 0U, 0U };            // لحظه‌ای که کاندیدا شروع شد
uint32_t g_hits[kEndstopAxes]      = { 0U, 0U, 0U };
bool     g_raw[kEndstopAxes]       = { false, false, false };   // آخرین سطح خام پین

bool     g_have_last   = false;   // آیا اجرای قبلی وجود دارد؟
uint32_t g_last_poll_ms = 0U;

bool     g_pulse_active   = false;   // پالس ساختگی در جریان است
uint32_t g_pulse_start_ms = 0U;
uint32_t g_pulse_len_ms   = 0U;
uint32_t g_log_ms         = 0U;      // لحظه آخرین لاگ موقت (بند ۶.۱)

void refresh_masks() {
    uint32_t raw_mask = 0U;
    uint32_t stable_mask = 0U;
    for (uint8_t i = 0U; i < kEndstopAxes; ++i) {
        if (g_raw[i])    { raw_mask    |= (1U << i); }
        if (g_stable[i]) { stable_mask |= (1U << i); }
    }
    mc_dbg_189_es_raw_mask    = raw_mask;
    mc_dbg_190_es_stable_mask = stable_mask;
    mc_dbg_191_es_hits_x      = g_hits[0];
    mc_dbg_192_es_hits_y      = g_hits[1];
    mc_dbg_193_es_hits_z      = g_hits[2];
}

}  // namespace

void mc_endstop_init() {
    const uint32_t now = mc_millis();
    for (uint8_t i = 0U; i < kEndstopAxes; ++i) {
        const bool level = mc_gpio_read(kPins[i]);
        g_raw[i]      = level;
        g_stable[i]   = level;    // اگر همین الان فشرده باشد، پایدار = فشرده (بدون شمردن در hits)
        g_cand[i]     = level;
        g_since_ms[i] = now;
        g_hits[i]     = 0U;
    }
    g_have_last      = false;
    g_last_poll_ms   = 0U;
    g_pulse_active   = false;
    g_pulse_start_ms = 0U;
    g_pulse_len_ms   = 0U;
    mc_dbg_194_es_glitches      = 0U;
    mc_dbg_195_es_fake_mask     = 0U;
    mc_dbg_196_es_fake_pulse_ms = 0U;
    mc_dbg_197_es_poll_runs     = 0U;
    mc_dbg_198_es_gap_max_ms    = 0U;
    refresh_masks();
}

void mc_endstop_poll() {
    const uint32_t now = mc_millis();

    // فاصله بین دو اجرای پشت‌سرهم (برای دانستن اینکه زمان‌بند چقدر دیر ما را صدا می‌زند)
    if (g_have_last) {
        const uint32_t gap = now - g_last_poll_ms;
        if (gap > mc_dbg_198_es_gap_max_ms) { mc_dbg_198_es_gap_max_ms = gap; }
    }
    g_last_poll_ms = now;
    g_have_last    = true;
    mc_dbg_197_es_poll_runs = mc_dbg_197_es_poll_runs + 1U;

    // اندستاپ ساختگی (برای تست بدون فشردن کلید)
    uint32_t fake = mc_dbg_195_es_fake_mask & 0x7U;

    // پالس ساختگی روی X: اولین اجرای بعد از نوشتن مقدار، پالس را شروع می‌کند.
    // همان اجرا یک نمونه HIGH می‌گیرد، پس پالس هیچ‌وقت کاملاً نادیده نمی‌ماند.
    const uint32_t pulse_req = mc_dbg_196_es_fake_pulse_ms;
    if (!g_pulse_active && (pulse_req != 0U)) {
        if (pulse_req <= kPulseMaxMs) {
            g_pulse_active   = true;
            g_pulse_start_ms = now;
            g_pulse_len_ms   = pulse_req;
        } else {
            mc_dbg_196_es_fake_pulse_ms = 0U;   // مقدار نامعتبر: رد شد
        }
    }
    if (g_pulse_active) {
        if ((now - g_pulse_start_ms) < g_pulse_len_ms) {
            fake |= 0x1U;   // بیت ۰ = X
        } else {
            g_pulse_active = false;
            mc_dbg_196_es_fake_pulse_ms = 0U;
        }
    }

    for (uint8_t i = 0U; i < kEndstopAxes; ++i) {
        const bool pin = mc_gpio_read(kPins[i]);
        g_raw[i] = pin;
        const bool sample = pin || (((fake >> i) & 0x1U) != 0U);

        if (sample != g_cand[i]) {
            // سطح عوض شد. اگر کاندیدای قبلی هنوز پذیرفته نشده بود، آن تغییر کوتاه بود و رد می‌شود.
            if (g_cand[i] != g_stable[i]) {
                mc_dbg_194_es_glitches = mc_dbg_194_es_glitches + 1U;
            }
            g_cand[i]     = sample;
            g_since_ms[i] = now;
        } else if ((g_cand[i] != g_stable[i]) && ((now - g_since_ms[i]) >= kEndstopDebounceMs)) {
            // سطح به‌اندازه کافی پایدار ماند: تغییر پذیرفته شد.
            g_stable[i] = g_cand[i];
            if (g_stable[i]) { g_hits[i] = g_hits[i] + 1U; }
        }
    }
    refresh_masks();

    // لاگ موقت برای تست بدون دیباگر (بند ۶.۱). در بند ۶.۴ با M119 جایگزین می‌شود.
    if ((now - g_log_ms) >= 1000U) {
        g_log_ms = now;
        mc_log_value(LogLevel::Info, "es_raw", mc_dbg_189_es_raw_mask);
        mc_log_value(LogLevel::Info, "es_stable", mc_dbg_190_es_stable_mask);
    }
}

bool mc_endstop_raw(uint8_t axis) {
    if (axis >= kEndstopAxes) { return false; }
    return g_raw[axis];
}

bool mc_endstop_triggered(uint8_t axis) {
    if (axis >= kEndstopAxes) { return false; }
    return g_stable[axis];
}

uint32_t mc_endstop_hits(uint8_t axis) {
    if (axis >= kEndstopAxes) { return 0U; }
    return g_hits[axis];
}

}  // namespace mc
