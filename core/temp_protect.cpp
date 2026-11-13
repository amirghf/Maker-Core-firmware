// حفاظت‌های watch heating، thermal runaway و heater failed (بند ۴.۷). هیچ هدر ST اینجا include نمی‌شود.
#include "temp_protect.h"
#include "thermistor.h"
#include "heater_ex.h"
#include "heater_bed.h"
#include "error.h"
#include "hal_time.h"

// متغیرهای debug؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_27_pt_watch_s   = 0;   // برای تست: دوره watch heating به ثانیه؛ ۰ = مقدار پیش‌فرض (حداکثر 3600)
volatile uint32_t mc_dbg_28_pt_runaway_s = 0;   // برای تست: دوره thermal runaway به ثانیه؛ ۰ = مقدار پیش‌فرض (حداکثر 3600)
volatile uint32_t mc_dbg_29_pt_failed_s  = 0;   // برای تست: حداکثر زمان heater failed به ثانیه؛ ۰ = مقدار پیش‌فرض (حداکثر 3600)
volatile uint32_t mc_dbg_30_pt_ex_state  = 0;   // وضعیت حفاظت اکسترودر: 0=Inactive 1=Heating 2=Stable
volatile uint32_t mc_dbg_31_pt_bed_state = 0;   // وضعیت حفاظت بستر: 0=Inactive 1=Heating 2=Stable
volatile uint32_t mc_dbg_32_pt_trip      = 0;   // کد خطایی که باعث kill شد؛ ۰ یعنی هنوز kill نشده
}

namespace mc {
namespace {

enum class PtState : uint8_t {
    Inactive = 0,   // هیتر مسلح نیست یا هدف صفر است
    Heating  = 1,   // در حال گرم‌شدن (هنوز به هدف نرسیده)
    Stable   = 2    // به هدف رسیده (حداکثر یک هیسترزیس پایین‌تر)
};

struct Params {
    uint32_t  watch_period_s;
    int32_t   watch_rise_x10;
    int32_t   runaway_hyst_x10;
    uint32_t  runaway_period_s;
    uint32_t  failed_period_s;
    ErrorCode watch_err;
    ErrorCode runaway_err;
    ErrorCode failed_err;
};

constexpr Params kExParams  = { kExWatchPeriodS,  kExWatchRiseX10,  kExRunawayHystX10,  kExRunawayPeriodS,
                                kExFailedPeriodS,
                                ErrorCode::WatchHeatingEx,  ErrorCode::ThermalRunawayEx,  ErrorCode::HeaterFailedEx };
constexpr Params kBedParams = { kBedWatchPeriodS, kBedWatchRiseX10, kBedRunawayHystX10, kBedRunawayPeriodS,
                                kBedFailedPeriodS,
                                ErrorCode::WatchHeatingBed, ErrorCode::ThermalRunawayBed, ErrorCode::HeaterFailedBed };

struct Channel {
    PtState  state            = PtState::Inactive;
    int32_t  last_target_x10  = 0;
    bool     watching         = false;
    int32_t  watch_goal_x10   = 0;
    uint32_t watch_start_ms   = 0;
    uint32_t failed_start_ms  = 0;
    bool     runaway_low      = false;
    uint32_t runaway_start_ms = 0;
};

Channel g_ex;
Channel g_bed;

[[noreturn]] void trip(ErrorCode code) {
    mc_dbg_32_pt_trip = static_cast<uint32_t>(code);
    mc_kill(code);
}

// مقدار debug صفر یعنی پیش‌فرض؛ غیر صفر یعنی همان مقدار (بین ۱ و ۳۶۰۰ ثانیه)
uint32_t pick_s(uint32_t default_s, uint32_t override_s) {
    if (override_s == 0U) { return default_s; }
    if (override_s > 3600U) { return 3600U; }
    return override_s;
}

void step(Channel& c, const Params& p, int32_t target_x10, bool have_temp, int32_t t_x10) {
    const uint32_t watch_s   = pick_s(p.watch_period_s,   mc_dbg_27_pt_watch_s);
    const uint32_t runaway_s = pick_s(p.runaway_period_s, mc_dbg_28_pt_runaway_s);
    const uint32_t failed_s  = pick_s(p.failed_period_s,  mc_dbg_29_pt_failed_s);
    const uint32_t now       = mc_millis();

    // هیتر درخواست نشده: همه چیز از نو
    if (target_x10 <= 0) {
        c = Channel{};
        return;
    }
    // سنسور سالم نیست: حفاظت ۴.۶ خودش kill می‌کند؛ اینجا زمان‌ها را عوض نمی‌کنیم
    if (!have_temp) { return; }

    // اولین بار یا هدف عوض شد: ارزیابی از نو
    if ((c.state == PtState::Inactive) || (target_x10 != c.last_target_x10)) {
        c.last_target_x10 = target_x10;
        c.watching        = false;
        c.runaway_low     = false;
        c.failed_start_ms = now;
        c.state = (t_x10 >= (target_x10 - p.runaway_hyst_x10)) ? PtState::Stable : PtState::Heating;
    }

    if (c.state == PtState::Heating) {
        if (t_x10 >= (target_x10 - p.runaway_hyst_x10)) {
            // به هدف رسید
            c.state       = PtState::Stable;
            c.watching    = false;
            c.runaway_low = false;
        } else {
            // heater failed: کل زمان گرم‌شدن
            if (mc_time_elapsed_ms(c.failed_start_ms, failed_s * 1000U)) { trip(p.failed_err); }

            // watch heating: فقط وقتی هنوز خیلی دور از هدف است
            const int32_t gap_x10 = target_x10 - t_x10;
            if (gap_x10 > (2 * p.watch_rise_x10)) {
                if (!c.watching) {
                    c.watching       = true;
                    c.watch_goal_x10 = t_x10 + p.watch_rise_x10;
                    c.watch_start_ms = now;
                } else if (t_x10 >= c.watch_goal_x10) {
                    c.watch_goal_x10 = t_x10 + p.watch_rise_x10;   // بالا رفت: پنجره از نو
                    c.watch_start_ms = now;
                } else if (mc_time_elapsed_ms(c.watch_start_ms, watch_s * 1000U)) {
                    trip(p.watch_err);
                }
            } else {
                c.watching = false;
            }
        }
    }

    if (c.state == PtState::Stable) {
        // thermal runaway: بعد از رسیدن به هدف، افتادن دما زیر حد
        if (t_x10 >= (target_x10 - p.runaway_hyst_x10)) {
            c.runaway_low = false;
        } else if (!c.runaway_low) {
            c.runaway_low      = true;
            c.runaway_start_ms = now;
        } else if (mc_time_elapsed_ms(c.runaway_start_ms, runaway_s * 1000U)) {
            trip(p.runaway_err);
        }
    }
}

}  // namespace

void mc_temp_protect_poll() {
    int32_t t = 0;
    bool ok = mc_temp_get_x10(AdcChannel::TempExtruder, &t);
    step(g_ex, kExParams, mc_heater_ex_target_x10(), ok, t);
    mc_dbg_30_pt_ex_state = static_cast<uint32_t>(g_ex.state);

    t = 0;
    ok = mc_temp_get_x10(AdcChannel::TempBed, &t);
    step(g_bed, kBedParams, mc_heater_bed_target_x10(), ok, t);
    mc_dbg_31_pt_bed_state = static_cast<uint32_t>(g_bed.state);
}

}  // namespace mc
