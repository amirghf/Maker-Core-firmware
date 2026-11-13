#pragma once
// صبر برای رسیدن دما به هدف (M109 و M190، بند ۴.۸). بدون حلقه انتظار: هر بار فقط یک بررسی انجام می‌شود.
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

namespace mc {

enum class WaitHeater : uint8_t {
    Extruder = 0,
    Bed      = 1
};

constexpr int32_t  kWaitWindowX10   = 10;       // پنجره ±۱ درجه
constexpr uint32_t kWaitResidencyMs = 10000U;   // باید این مدت پیوسته داخل پنجره بماند
constexpr uint32_t kWaitReportMs    = 1000U;    // هر ۱ ثانیه یک خط گزارش دما

// شروع صبر. target_x10 هدفِ اعمال‌شده (بعد از سقف) و بزرگ‌تر از صفر است.
// wait_cooling = false (دستور S): «رسیده» یعنی دما >= هدف - پنجره (بالاتر از هدف هم رسیده حساب می‌شود).
// wait_cooling = true (دستور R): «رسیده» یعنی |دما - هدف| <= پنجره.
void mc_temp_wait_begin(WaitHeater heater, int32_t target_x10, bool wait_cooling);

// هر بار از تابع دستور صدا زده می‌شود. true = صبر تمام شد (یا صبری در جریان نیست).
// هر ۱ ثانیه یک خط "T:... /... B:... /... @:... B@:... W:?" (بدون ok) برای کامپیوتر می‌فرستد.
bool mc_temp_wait_poll();

// آیا الان صبری در جریان است؟
bool mc_temp_wait_active();

}  // namespace mc
