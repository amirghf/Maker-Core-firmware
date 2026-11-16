// حدود سرعت و شتاب حرکت (بند ۵.۱۱). فقط حالت و عدد نگه می‌دارد و به سخت‌افزار دست نمی‌زند.
#include "motion_limits.h"

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند. شماره‌ها دائمی‌اند.
extern "C" {
volatile uint32_t mc_dbg_173_ml_feed_x  = 0;   // خواندنی: حداکثر سرعت X به mm/s ضرب در 1000.
volatile uint32_t mc_dbg_174_ml_feed_y  = 0;   // Y.
volatile uint32_t mc_dbg_175_ml_feed_z  = 0;   // Z.
volatile uint32_t mc_dbg_176_ml_feed_e  = 0;   // E.
volatile uint32_t mc_dbg_177_ml_acc_prt = 0;   // خواندنی: شتاب print (mm/s² گردشده).
volatile uint32_t mc_dbg_178_ml_acc_ret = 0;   // شتاب retract.
volatile uint32_t mc_dbg_179_ml_acc_trv = 0;   // شتاب travel.
volatile uint32_t mc_dbg_180_ml_minfeed = 0;   // خواندنی: حداقل سرعت (M205 S) به mm/s ضرب در 1000.
volatile uint32_t mc_dbg_181_ml_refused = 0;   // خواندنی: تعداد M203/M204/M205 ردشده.
}

namespace mc {
namespace {

float g_feed[4]  = { kMlimDefaultFeedXMmS, kMlimDefaultFeedYMmS,
                     kMlimDefaultFeedZMmS, kMlimDefaultFeedEMmS };
float g_acc[3]   = { kMlimDefaultAccPrint, kMlimDefaultAccRetract, kMlimDefaultAccTravel };
float g_minfeed  = kMlimDefaultMinFeedMmS;

uint32_t to_milli(float v) {
    return static_cast<uint32_t>((v * 1000.0f) + 0.5f);
}

void refresh_debug() {
    mc_dbg_173_ml_feed_x  = to_milli(g_feed[0]);
    mc_dbg_174_ml_feed_y  = to_milli(g_feed[1]);
    mc_dbg_175_ml_feed_z  = to_milli(g_feed[2]);
    mc_dbg_176_ml_feed_e  = to_milli(g_feed[3]);
    mc_dbg_177_ml_acc_prt = static_cast<uint32_t>(g_acc[0] + 0.5f);
    mc_dbg_178_ml_acc_ret = static_cast<uint32_t>(g_acc[1] + 0.5f);
    mc_dbg_179_ml_acc_trv = static_cast<uint32_t>(g_acc[2] + 0.5f);
    mc_dbg_180_ml_minfeed = to_milli(g_minfeed);
}

}  // namespace

void mc_mlim_init() {
    g_feed[0] = kMlimDefaultFeedXMmS;
    g_feed[1] = kMlimDefaultFeedYMmS;
    g_feed[2] = kMlimDefaultFeedZMmS;
    g_feed[3] = kMlimDefaultFeedEMmS;
    g_acc[0]  = kMlimDefaultAccPrint;
    g_acc[1]  = kMlimDefaultAccRetract;
    g_acc[2]  = kMlimDefaultAccTravel;
    g_minfeed = kMlimDefaultMinFeedMmS;
    mc_dbg_181_ml_refused = 0U;
    refresh_debug();
}

void mc_mlim_poll() {
    refresh_debug();
}

float mc_mlim_feed_get(uint8_t axis) {
    if (axis >= 4U) { return 1.0f; }
    return g_feed[axis];
}

bool mc_mlim_feed_set(uint8_t axis, float mm_s) {
    if (axis >= 4U) { return false; }
    if (!((mm_s >= kMlimMinFeedMmS) && (mm_s <= kMlimMaxFeedMmS))) { return false; }   // NaN رد می‌شود
    g_feed[axis] = mm_s;
    refresh_debug();
    return true;
}

float mc_mlim_acc_get(uint8_t kind) {
    if (kind >= 3U) { return 100.0f; }
    return g_acc[kind];
}

bool mc_mlim_acc_set(uint8_t kind, float mm_s2) {
    if (kind >= 3U) { return false; }
    if (!((mm_s2 >= kMlimMinAccMmS2) && (mm_s2 <= kMlimMaxAccMmS2))) { return false; }   // NaN رد می‌شود
    g_acc[kind] = mm_s2;
    refresh_debug();
    return true;
}

float mc_mlim_minfeed_get() {
    return g_minfeed;
}

bool mc_mlim_minfeed_set(float mm_s) {
    if (!((mm_s >= kMlimMinMinFeedMmS) && (mm_s <= kMlimMaxMinFeedMmS))) { return false; }   // NaN رد می‌شود
    g_minfeed = mm_s;
    refresh_debug();
    return true;
}

void mc_mlim_note_refused() {
    mc_dbg_181_ml_refused = mc_dbg_181_ml_refused + 1U;
}

}  // namespace mc
