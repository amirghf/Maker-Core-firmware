// حالت‌ها و واحدهای حرکت (بند ۵.۸). فقط حالت و عدد نگه می‌دارد و به سخت‌افزار دست نمی‌زند.
#include "motion_modes.h"
#include "../config/mc_config.h"

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_135_mm_dist_mode = 0;   // خواندنی: ۰ = مطلق (G90)، ۱ = نسبی (G91)
volatile uint32_t mc_dbg_136_mm_e_rel     = 0;   // خواندنی: حالت مؤثر E؛ ۰ = مطلق، ۱ = نسبی (G91 یا M83)
volatile uint32_t mc_dbg_137_mm_spmm_x    = 0;   // خواندنی: گام بر میلی‌متر X ضرب در ۱۰۰۰
volatile uint32_t mc_dbg_138_mm_spmm_y    = 0;   // خواندنی: گام بر میلی‌متر Y ضرب در ۱۰۰۰
volatile uint32_t mc_dbg_139_mm_spmm_z    = 0;   // خواندنی: گام بر میلی‌متر Z ضرب در ۱۰۰۰
volatile uint32_t mc_dbg_140_mm_spmm_e    = 0;   // خواندنی: گام بر میلی‌متر E ضرب در ۱۰۰۰
volatile uint32_t mc_dbg_141_mm_feed      = 0;   // خواندنی: فید فعلی (میلی‌متر بر دقیقه، گردشده)
volatile uint32_t mc_dbg_142_mm_feed_set  = 0;   // نوشتنی: عدد غیرصفر ← فید را روی همین مقدار (میلی‌متر بر دقیقه) می‌گذارد و خودش صفر می‌شود
volatile uint32_t mc_dbg_143_mm_refused   = 0;   // خواندنی: تعداد دستورها یا نوشتن‌های ردشده (مقدار نامعتبر)
volatile uint32_t mc_dbg_144_mm_applied   = 0;   // خواندنی: تعداد دستورها یا نوشتن‌های پذیرفته‌شده (G90 G91 G92 M82 M83 M92 و فید)
}

namespace mc {
namespace {

bool  g_relative   = false;   // G90 / G91
bool  g_e_relative = false;   // M82 / M83
float g_spmm[4]    = { kModeDefaultStepsPerMm[0], kModeDefaultStepsPerMm[1],
                       kModeDefaultStepsPerMm[2], kModeDefaultStepsPerMm[3] };
float g_feed_mmin  = kModeDefaultFeedMmMin;

uint32_t to_milli(float v) {
    return static_cast<uint32_t>((v * 1000.0f) + 0.5f);
}

void refresh_debug() {
    mc_dbg_135_mm_dist_mode = g_relative ? 1U : 0U;
    mc_dbg_136_mm_e_rel     = mc_mode_e_relative_get() ? 1U : 0U;
    mc_dbg_137_mm_spmm_x    = to_milli(g_spmm[0]);
    mc_dbg_138_mm_spmm_y    = to_milli(g_spmm[1]);
    mc_dbg_139_mm_spmm_z    = to_milli(g_spmm[2]);
    mc_dbg_140_mm_spmm_e    = to_milli(g_spmm[3]);
    mc_dbg_141_mm_feed      = static_cast<uint32_t>(g_feed_mmin + 0.5f);
}

}  // namespace

void mc_mode_init() {
    g_relative   = false;
    g_e_relative = false;
    for (uint8_t i = 0U; i < 4U; ++i) { g_spmm[i] = kModeDefaultStepsPerMm[i]; }
    g_feed_mmin = kModeDefaultFeedMmMin;
    mc_dbg_142_mm_feed_set = 0U;
    mc_dbg_143_mm_refused  = 0U;
    mc_dbg_144_mm_applied  = 0U;
    refresh_debug();
}

void mc_mode_poll() {
    const uint32_t req = mc_dbg_142_mm_feed_set;
    if (req != 0U) {
        mc_dbg_142_mm_feed_set = 0U;
        if (mc_mode_feed_set(static_cast<float>(req))) {
            mc_mode_note_applied();
        } else {
            mc_mode_note_refused();
        }
    }
    refresh_debug();
}

bool mc_mode_relative_get() { return g_relative; }
void mc_mode_relative_set(bool relative) { g_relative = relative; refresh_debug(); }

bool mc_mode_e_relative_get() { return g_relative || g_e_relative; }
void mc_mode_e_relative_set(bool relative) { g_e_relative = relative; refresh_debug(); }

float mc_mode_spmm_get(uint8_t axis) {
    if (axis >= 4U) { return kModeDefaultStepsPerMm[0]; }
    return g_spmm[axis];
}

bool mc_mode_spmm_set(uint8_t axis, float value) {
    if (axis >= 4U) { return false; }
    if (!((value >= kModeMinStepsPerMm) && (value <= kModeMaxStepsPerMm))) { return false; }   // NaN هم رد می‌شود
    g_spmm[axis] = value;
    refresh_debug();
    return true;
}

float mc_mode_feed_get_mmin() { return g_feed_mmin; }
float mc_mode_feed_get_mms() { return g_feed_mmin / 60.0f; }

bool mc_mode_feed_set(float mm_per_min) {
    if (!((mm_per_min >= kModeMinFeedMmMin) && (mm_per_min <= kModeMaxFeedMmMin))) { return false; }
    g_feed_mmin = mm_per_min;
    refresh_debug();
    return true;
}

void mc_mode_note_applied() { mc_dbg_144_mm_applied = mc_dbg_144_mm_applied + 1U; }
void mc_mode_note_refused() { mc_dbg_143_mm_refused = mc_dbg_143_mm_refused + 1U; }

}  // namespace mc
