// محافظ اندستاپ (بند ۶.۲).
#include "endstop_guard.h"
#include "step_gen.h"
#include "motion_queue.h"
#include "axis_move.h"
#include "machine_state.h"
#include "motion_modes.h"
#include "log.h"
#include <math.h>

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند. شماره‌ها دائمی‌اند.
extern "C" {
volatile uint32_t mc_dbg_199_eg_hits      = 0;  // خواندنی: تعداد کل توقف‌های اندستاپ از روشن‌شدن برد
volatile uint32_t mc_dbg_200_eg_last_mask = 0;  // خواندنی: ماسک محورهای آخرین توقف (بیت ۰ = X ، ۱ = Y ، ۲ = Z)
volatile uint32_t mc_dbg_201_eg_resyncs   = 0;  // خواندنی: تعداد دفعاتی که موقعیت منطقی از گام‌های واقعی دوباره ساخته شد
volatile int32_t  mc_dbg_202_eg_off_x     = 0;  // خواندنی: اختلاف ذخیره‌شده (موقعیت منطقی منهای گام واقعی) محور X بر حسب گام
volatile int32_t  mc_dbg_203_eg_off_y     = 0;  // خواندنی: همان برای Y
volatile int32_t  mc_dbg_204_eg_off_z     = 0;  // خواندنی: همان برای Z
volatile int32_t  mc_dbg_205_eg_off_e     = 0;  // خواندنی: همان برای E
}

namespace mc {
namespace {

int32_t  g_off[4]    = { 0, 0, 0, 0 };   // اختلاف موقعیت منطقی و گام واقعی هر محور (گام)
uint32_t g_seen_hits = 0U;               // تعداد توقف‌هایی که پردازش شده‌اند

void publish() {
    mc_dbg_199_eg_hits      = mc_stepgen_endstop_hits();
    mc_dbg_200_eg_last_mask = mc_stepgen_endstop_last_mask();
    mc_dbg_202_eg_off_x     = g_off[0];
    mc_dbg_203_eg_off_y     = g_off[1];
    mc_dbg_204_eg_off_z     = g_off[2];
    mc_dbg_205_eg_off_e     = g_off[3];
}

}  // namespace

void mc_eguard_init() {
    for (uint8_t i = 0U; i < 4U; ++i) { g_off[i] = 0; }
    g_seen_hits = mc_stepgen_endstop_hits();
    mc_dbg_201_eg_resyncs = 0U;
    publish();
}

void mc_eguard_snapshot() {
    for (uint8_t i = 0U; i < 4U; ++i) {
        const float spmm = mc_mode_spmm_get(i);
        const int32_t logical_steps = static_cast<int32_t>(lroundf(mc_position_get(i) * spmm));
        g_off[i] = logical_steps - mc_axis_position_steps(static_cast<StepAxis>(i));
    }
    publish();
}

void mc_eguard_resync() {
    for (uint8_t i = 0U; i < 4U; ++i) {
        const float spmm = mc_mode_spmm_get(i);
        const int32_t steps = mc_axis_position_steps(static_cast<StepAxis>(i));
        mc_position_set(i, static_cast<float>(steps + g_off[i]) / spmm);
    }
    publish();
}

void mc_eguard_poll() {
    const uint32_t hits = mc_stepgen_endstop_hits();
    if (hits == g_seen_hits) {
        publish();
        return;
    }
    g_seen_hits = hits;
    // مولد گام قبلاً داخل وقفه متوقف شده؛ صف و بقیه بلوک‌ها را پاک کن (حرکت جاری بسته و گام‌هایش جمع می‌شود)
    mc_mq_clear();
    // موقعیت منطقی = (گام واقعی + اختلاف ذخیره‌شده) ÷ گام بر میلی‌متر
    for (uint8_t i = 0U; i < 4U; ++i) {
        const float spmm = mc_mode_spmm_get(i);
        const int32_t steps = mc_axis_position_steps(static_cast<StepAxis>(i));
        mc_position_set(i, static_cast<float>(steps + g_off[i]) / spmm);
    }
    mc_dbg_201_eg_resyncs = mc_dbg_201_eg_resyncs + 1U;
    publish();
    mc_log_value(LogLevel::Warn, "endstop hit mask", mc_stepgen_endstop_last_mask());
}

}  // namespace mc
