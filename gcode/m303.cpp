// M303: تنظیم خودکار PID هیتر اکسترودر (بند ۴.۹). M303 [E0] [S<درجه>] [C<چرخه>] [U<0 یا 1>]
// S پیش‌فرض 200 (مجاز بین 50 و سقف هدف). C پیش‌فرض 5 (به بازه 3 تا 20 محدود می‌شود). U1: ضرایب اعمال شود (تا ریست).
// E فقط 0 (بستر PID ندارد). دستور تا پایان Busy است. در پایان (موفق یا ناموفق) هدف هیتر ۰ می‌شود.
#include "gcode_handlers.h"
#include "heater_ex.h"
#include "pid_autotune.h"
#include "temp_report.h"
#include "temp_param.h"
#include "thermistor.h"
#include "text_format.h"
#include "host_reply.h"
#include "hal_time.h"
#include "log.h"

namespace mc {
namespace {

constexpr float    kDefaultTargetC = 200.0f;
constexpr uint32_t kReportMs       = 1000U;   // هر ۱ ثانیه یک خط گزارش دما

bool     g_apply     = false;   // U1
uint8_t  g_seen_cycle = 0U;     // آخرین چرخه‌ای که خط پیشرفتش فرستاده شده
uint32_t g_rep_ms    = 0U;

const char* fail_text(AutotuneFail f) {
    switch (f) {
        case AutotuneFail::OverTemp:     return "temperature too high";
        case AutotuneFail::Timeout:      return "timeout";
        case AutotuneFail::Sensor:       return "sensor error";
        case AutotuneFail::GainsInvalid: return "gains out of range";
        default:                         return "unknown";
    }
}

// " Kp:xx.xx Ki:xx.xx Kd:xx.xx" را به متن اضافه می‌کند
uint16_t put_gains(char* dst, uint16_t cap, uint16_t pos, const AutotuneResult& r) {
    pos = mc_fmt_text(dst, cap, pos, " Kp:");
    pos = mc_fmt_float(dst, cap, pos, r.kp, 2);
    pos = mc_fmt_text(dst, cap, pos, " Ki:");
    pos = mc_fmt_float(dst, cap, pos, r.ki, 2);
    pos = mc_fmt_text(dst, cap, pos, " Kd:");
    pos = mc_fmt_float(dst, cap, pos, r.kd, 2);
    return pos;
}

uint32_t to_x100(float v) {
    if (!(v > 0.0f)) { return 0U; }
    if (v > 4000000.0f) { v = 4000000.0f; }
    return static_cast<uint32_t>((v * 100.0f) + 0.5f);
}

}  // namespace

GcodeStatus mc_gcode_m303(const GcodeCommand& cmd, bool first_call) {
    if (first_call) {
        if (mc_gcode_has_value(cmd, 'E') && (mc_gcode_int(cmd, 'E', 0) != 0)) {
            mc_log(LogLevel::Warn, "M303 only extruder E0 (bed has no PID)");
            return GcodeStatus::Error;
        }
        if (mc_heater_ex_test_mode()) {
            mc_log(LogLevel::Warn, "M303 refused: heater in debug test mode");
            return GcodeStatus::Error;
        }
        const int32_t target = mc_temp_c_to_x10(mc_gcode_value(cmd, 'S', kDefaultTargetC));
        if ((target < kAutotuneMinTargetX10) || (target > kHeaterExTargetCapX10)) {
            mc_log(LogLevel::Warn, "M303 target out of range");
            return GcodeStatus::Error;
        }
        int32_t cycles = mc_gcode_int(cmd, 'C', static_cast<int32_t>(kAutotuneCyclesDefault));
        if (cycles < static_cast<int32_t>(kAutotuneCyclesMin)) { cycles = static_cast<int32_t>(kAutotuneCyclesMin); }
        if (cycles > static_cast<int32_t>(kAutotuneCyclesMax)) { cycles = static_cast<int32_t>(kAutotuneCyclesMax); }
        g_apply = (mc_gcode_int(cmd, 'U', 0) != 0);

        int32_t t_now = 0;
        if (!mc_temp_get_x10(AdcChannel::TempExtruder, &t_now)) {
            mc_log(LogLevel::Warn, "M303 refused: extruder sensor not ok");
            return GcodeStatus::Error;
        }

        (void)mc_heater_ex_set_target_x10(0);
        mc_autotune_begin(target, static_cast<uint8_t>(cycles));
        g_seen_cycle = 0U;
        g_rep_ms = mc_millis();
        mc_reply_line("PID Autotune start");
    }

    const AutotuneState st = mc_autotune_state();

    if (st == AutotuneState::Done) {
        AutotuneResult r = {0.0f, 0.0f, 0.0f};
        (void)mc_autotune_latest(&r);
        char buf[96];
        const uint16_t cap = static_cast<uint16_t>(sizeof(buf));
        uint16_t n = 0U;
        n = mc_fmt_text(buf, cap, n, "PID Autotune finished:");
        n = put_gains(buf, cap, n, r);
        mc_reply_line(buf);
        if (g_apply) {
            mc_heater_ex_set_gains_x100(to_x100(r.kp), to_x100(r.ki), to_x100(r.kd));
            mc_reply_line("PID gains applied (until reset)");
        } else {
            mc_reply_line("PID gains not applied (use U1 to apply)");
        }
        mc_autotune_release();
        (void)mc_heater_ex_set_target_x10(0);
        return GcodeStatus::Done;
    }

    if (st == AutotuneState::Failed) {
        char buf[96];
        const uint16_t cap = static_cast<uint16_t>(sizeof(buf));
        uint16_t n = 0U;
        n = mc_fmt_text(buf, cap, n, "PID Autotune failed: ");
        n = mc_fmt_text(buf, cap, n, fail_text(mc_autotune_fail()));
        mc_reply_line(buf);
        mc_log(LogLevel::Warn, "M303 autotune failed");
        mc_autotune_release();
        (void)mc_heater_ex_set_target_x10(0);
        return GcodeStatus::Error;
    }

    if (st == AutotuneState::Idle) {
        return GcodeStatus::Error;   // نباید پیش بیاید (دفاعی)
    }

    // در جریان: بعد از هر چرخه یک خط پیشرفت
    const uint8_t cyc = mc_autotune_cycle();
    if (cyc != g_seen_cycle) {
        g_seen_cycle = cyc;
        AutotuneResult r = {0.0f, 0.0f, 0.0f};
        if (mc_autotune_latest(&r)) {
            char buf[96];
            const uint16_t cap = static_cast<uint16_t>(sizeof(buf));
            uint16_t n = 0U;
            n = mc_fmt_text(buf, cap, n, "PID Autotune cycle ");
            n = mc_fmt_uint(buf, cap, n, cyc);
            n = mc_fmt_text(buf, cap, n, "/");
            n = mc_fmt_uint(buf, cap, n, mc_autotune_cycles_total());
            n = mc_fmt_text(buf, cap, n, ":");
            n = put_gains(buf, cap, n, r);
            mc_reply_line(buf);
        }
    }

    // هر ۱ ثانیه گزارش دما (مثل M109)
    if (mc_time_elapsed_ms(g_rep_ms, kReportMs)) {
        g_rep_ms = mc_millis();
        char buf[96];
        const uint16_t cap = static_cast<uint16_t>(sizeof(buf));
        uint16_t n = mc_temp_report_build(buf, cap);
        (void)mc_fmt_text(buf, cap, n, " W:?");
        mc_reply_line(buf);
    }
    return GcodeStatus::Busy;
}

}  // namespace mc
