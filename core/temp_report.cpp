#include "temp_report.h"
#include "temp_wait.h"
#include "pid_autotune.h"
#include "thermistor.h"
#include "heater_ex.h"
#include "heater_bed.h"
#include "output_pwm.h"
#include "text_format.h"
#include "host_reply.h"
#include "hal_time.h"

// متغیرهای debug؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_36_autorep_s = 0;   // فاصله گزارش خودکار دما (ثانیه)؛ ۰ = خاموش
}

namespace mc {
namespace {

uint8_t  g_interval_s = 0U;
uint32_t g_last_ms    = 0U;

// عدد ×۱۰ را به شکل 25.0 می‌نویسد (منفی هم درست)
uint16_t put_x10(char* dst, uint16_t cap, uint16_t pos, int32_t v) {
    if (v < 0) {
        pos = mc_fmt_text(dst, cap, pos, "-");
        v = -v;
    }
    pos = mc_fmt_uint(dst, cap, pos, static_cast<uint32_t>(v / 10));
    pos = mc_fmt_text(dst, cap, pos, ".");
    return mc_fmt_uint(dst, cap, pos, static_cast<uint32_t>(v % 10));
}

}  // namespace

uint16_t mc_temp_report_build(char* dst, uint16_t cap) {
    int32_t t = 0;
    uint16_t n = 0U;
    n = mc_fmt_text(dst, cap, n, "T:");
    if (!mc_temp_get_x10(AdcChannel::TempExtruder, &t)) { t = 0; }
    n = put_x10(dst, cap, n, t);
    n = mc_fmt_text(dst, cap, n, " /");
    n = put_x10(dst, cap, n, mc_heater_ex_target_x10());
    n = mc_fmt_text(dst, cap, n, " B:");
    t = 0;
    if (!mc_temp_get_x10(AdcChannel::TempBed, &t)) { t = 0; }
    n = put_x10(dst, cap, n, t);
    n = mc_fmt_text(dst, cap, n, " /");
    n = put_x10(dst, cap, n, mc_heater_bed_target_x10());
    n = mc_fmt_text(dst, cap, n, " @:");
    n = mc_fmt_uint(dst, cap, n, mc_output_get(OutputId::HeaterEx));
    n = mc_fmt_text(dst, cap, n, " B@:");
    n = mc_fmt_uint(dst, cap, n, mc_output_get(OutputId::HeaterBed));
    return n;
}

void mc_temp_autoreport_set(uint8_t seconds) {
    g_interval_s = seconds;
    g_last_ms = mc_millis();
    mc_dbg_36_autorep_s = seconds;
}

void mc_temp_autoreport_poll() {
    mc_dbg_36_autorep_s = g_interval_s;
    if (g_interval_s == 0U) { return; }
    // وسط M109 و M190 و M303 خود آن دستور گزارش می‌دهد؛ گزارش تکراری نفرست
    if (mc_temp_wait_active() || mc_autotune_active()) {
        g_last_ms = mc_millis();
        return;
    }
    if (!mc_time_elapsed_ms(g_last_ms, static_cast<uint32_t>(g_interval_s) * 1000U)) { return; }
    g_last_ms = mc_millis();
    char buf[96];
    (void)mc_temp_report_build(buf, static_cast<uint16_t>(sizeof(buf)));
    mc_reply_line(buf);
}

}  // namespace mc
