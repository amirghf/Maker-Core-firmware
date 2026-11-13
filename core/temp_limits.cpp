// حداقل و حداکثر دما (بند ۴.۶). هیچ هدر ST اینجا include نمی‌شود.
#include "temp_limits.h"
#include "thermistor.h"
#include "heater_ex.h"
#include "heater_bed.h"
#include "error.h"

// متغیرهای debug؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile int32_t  mc_dbg_tlim_ex_min_x10  = 50;     // حداقل اکسترودر ×۱۰ (فقط برای نمایش)
volatile int32_t  mc_dbg_tlim_ex_max_x10  = 2750;   // حداکثر اکسترودر ×۱۰ (فقط برای نمایش)
volatile int32_t  mc_dbg_tlim_bed_min_x10 = 50;     // حداقل بستر ×۱۰ (فقط برای نمایش)
volatile int32_t  mc_dbg_tlim_bed_max_x10 = 1300;   // حداکثر بستر ×۱۰ (فقط برای نمایش)
volatile uint32_t mc_dbg_tlim_trip        = 0;      // کد خطایی که باعث kill شد؛ ۰ یعنی هنوز kill نشده
}

namespace mc {
namespace {

constexpr uint8_t kTripPolls = 3;   // شرط باید این‌قدر دور ۱۰۰ میلی‌ثانیه‌ای پشت‌سرهم بماند

uint8_t g_ex_max_cnt  = 0;
uint8_t g_ex_min_cnt  = 0;
uint8_t g_bed_max_cnt = 0;
uint8_t g_bed_min_cnt = 0;

[[noreturn]] void trip(ErrorCode code) {
    mc_dbg_tlim_trip = static_cast<uint32_t>(code);
    mc_kill(code);
}

void check_channel(AdcChannel ch, bool demanded, int32_t min_x10, int32_t max_x10,
                   ErrorCode min_err, ErrorCode max_err, uint8_t* max_cnt, uint8_t* min_cnt) {
    int32_t t = 0;
    const bool ok = mc_temp_get_x10(ch, &t);

    // حداکثر: همیشه فعال
    if (ok && (t >= max_x10)) {
        *max_cnt = static_cast<uint8_t>(*max_cnt + 1U);
        if (*max_cnt >= kTripPolls) { trip(max_err); }
    } else {
        *max_cnt = 0U;
    }

    // حداقل: فقط وقتی هیتر درخواست‌شده است
    bool low = false;
    if (demanded) {
        const TempStatus st = mc_temp_status(ch);
        if ((st == TempStatus::Open) || (st == TempStatus::Short)) {
            low = true;
        } else if (ok && (t < min_x10)) {
            low = true;
        }
    }
    if (low) {
        *min_cnt = static_cast<uint8_t>(*min_cnt + 1U);
        if (*min_cnt >= kTripPolls) { trip(min_err); }
    } else {
        *min_cnt = 0U;
    }
}

}  // namespace

void mc_temp_limits_poll() {
    check_channel(AdcChannel::TempExtruder, mc_heater_ex_demanded(),
                  kTempExMinX10, kTempExMaxX10,
                  ErrorCode::MinTempExtruder, ErrorCode::MaxTempExtruder,
                  &g_ex_max_cnt, &g_ex_min_cnt);
    check_channel(AdcChannel::TempBed, mc_heater_bed_demanded(),
                  kTempBedMinX10, kTempBedMaxX10,
                  ErrorCode::MinTempBed, ErrorCode::MaxTempBed,
                  &g_bed_max_cnt, &g_bed_min_cnt);
}

}  // namespace mc
