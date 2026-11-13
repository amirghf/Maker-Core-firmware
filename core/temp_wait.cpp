#include "temp_wait.h"
#include "temp_report.h"
#include "thermistor.h"
#include "text_format.h"
#include "host_reply.h"
#include "hal_time.h"

// متغیرهای debug؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_37_wait_state    = 0;   // صبر M109 یا M190: 0=بیکار 1=اکسترودر 2=بستر
volatile uint32_t mc_dbg_38_wait_resid_ms = 0;   // چند میلی‌ثانیه است که دما پیوسته داخل پنجره است (۰ = بیرون پنجره)
}

namespace mc {
namespace {

bool       g_active  = false;
WaitHeater g_heater  = WaitHeater::Extruder;
int32_t    g_target  = 0;
bool       g_cool    = false;
bool       g_in_win  = false;
uint32_t   g_since   = 0U;
uint32_t   g_rep_ms  = 0U;

void finish() {
    g_active = false;
    g_in_win = false;
    mc_dbg_37_wait_state    = 0U;
    mc_dbg_38_wait_resid_ms = 0U;
}

}  // namespace

void mc_temp_wait_begin(WaitHeater heater, int32_t target_x10, bool wait_cooling) {
    g_active = true;
    g_heater = heater;
    g_target = target_x10;
    g_cool   = wait_cooling;
    g_in_win = false;
    g_rep_ms = mc_millis();
    mc_dbg_37_wait_state    = (heater == WaitHeater::Extruder) ? 1U : 2U;
    mc_dbg_38_wait_resid_ms = 0U;
}

bool mc_temp_wait_active() {
    return g_active;
}

bool mc_temp_wait_poll() {
    if (!g_active) { return true; }
    const uint32_t now = mc_millis();

    const AdcChannel ch = (g_heater == WaitHeater::Extruder) ? AdcChannel::TempExtruder : AdcChannel::TempBed;
    int32_t t = 0;
    bool reached = false;
    if (mc_temp_get_x10(ch, &t)) {
        const int32_t diff = t - g_target;
        reached = g_cool ? ((diff <= kWaitWindowX10) && (diff >= -kWaitWindowX10))
                         : (diff >= -kWaitWindowX10);
    }   // سنسور سالم نیست: «نرسیده»؛ حفاظت ۴.۶ خودش kill می‌کند

    if (reached) {
        if (!g_in_win) {
            g_in_win = true;
            g_since  = now;
        }
        const uint32_t resid = now - g_since;
        mc_dbg_38_wait_resid_ms = resid;
        if (resid >= kWaitResidencyMs) {
            finish();
            return true;
        }
    } else {
        g_in_win = false;
        mc_dbg_38_wait_resid_ms = 0U;
    }

    if (mc_time_elapsed_ms(g_rep_ms, kWaitReportMs)) {
        g_rep_ms = now;
        char buf[96];
        uint16_t n = mc_temp_report_build(buf, static_cast<uint16_t>(sizeof(buf)));
        (void)mc_fmt_text(buf, static_cast<uint16_t>(sizeof(buf)), n, " W:?");
        mc_reply_line(buf);
    }
    return false;
}

}  // namespace mc
