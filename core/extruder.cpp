// اکسترودر (بند ۵.۹): فقط تصمیم «مجاز بودن اکستروژن» و حداقل دما. به سخت‌افزار دست نمی‌زند.
#include "extruder.h"
#include "thermistor.h"
#include "hal_adc.h"

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_145_ex_min_t10   = 0;   // خواندنی: حداقل دمای اکستروژن ×۱۰ (پیش‌فرض 1700)
volatile uint32_t mc_dbg_146_ex_allowed   = 0;   // خواندنی: ۱ = اکستروژن الان مجاز است، ۰ = ممنوع
volatile uint32_t mc_dbg_147_ex_cold_ok   = 0;   // خواندنی: ۱ = اکستروژن سرد مجاز است (M302 P1)
volatile uint32_t mc_dbg_148_ex_fake_en   = 0;   // نوشتنی: ۱ = به‌جای دمای واقعی، دمای ساختگی mc_dbg_149 استفاده شود (فقط برای همین بررسی؛ به هیتر و حفاظت‌ها ربطی ندارد)
volatile uint32_t mc_dbg_149_ex_fake_t10  = 0;   // نوشتنی: دمای ساختگی ×۱۰ (مثلاً 2000 = 200 درجه)
volatile uint32_t mc_dbg_150_ex_refused   = 0;   // خواندنی: تعداد بلوک‌های ردشده به‌خاطر اکستروژن سرد
volatile uint32_t mc_dbg_151_ex_temp_t10  = 0;   // خواندنی: دمایی که بررسی همین الان از آن استفاده می‌کند ×۱۰ (واقعی یا ساختگی)؛ ۰ اگر سنسور سالم نیست
}

namespace mc {
namespace {

int32_t g_min_temp_x10 = kExDefaultMinTempX10;   // حداقل دمای اکستروژن ×۱۰
bool    g_cold_ok      = false;                  // M302 P1 ← true

// دمای مورد استفاده بررسی. false اگر دمای معتبر نداریم.
bool read_temp(int32_t* t_x10) {
    if (mc_dbg_148_ex_fake_en == 1U) {
        *t_x10 = static_cast<int32_t>(mc_dbg_149_ex_fake_t10);
        return true;
    }
    return mc_temp_get_x10(AdcChannel::TempExtruder, t_x10);
}

}  // namespace

void mc_extruder_init() {
    g_min_temp_x10 = kExDefaultMinTempX10;
    g_cold_ok      = false;
    mc_dbg_148_ex_fake_en  = 0U;
    mc_dbg_149_ex_fake_t10 = 0U;
    mc_dbg_150_ex_refused  = 0U;
    mc_extruder_poll();
}

void mc_extruder_poll() {
    int32_t t = 0;
    const bool ok = read_temp(&t);
    mc_dbg_145_ex_min_t10  = static_cast<uint32_t>(g_min_temp_x10);
    mc_dbg_146_ex_allowed  = mc_extruder_allowed() ? 1U : 0U;
    mc_dbg_147_ex_cold_ok  = g_cold_ok ? 1U : 0U;
    mc_dbg_151_ex_temp_t10 = (ok && (t > 0)) ? static_cast<uint32_t>(t) : 0U;
}

bool mc_extruder_allowed() {
    if (g_cold_ok) { return true; }
    int32_t t = 0;
    if (!read_temp(&t)) { return false; }   // سنسور سالم نیست
    return t >= g_min_temp_x10;
}

int32_t mc_extruder_min_temp_x10_get() { return g_min_temp_x10; }

bool mc_extruder_min_temp_x10_set(int32_t t_x10) {
    if ((t_x10 < 0) || (t_x10 > (kExMaxMinTempDeg * 10))) { return false; }
    g_min_temp_x10 = t_x10;
    return true;
}

bool mc_extruder_cold_ok_get() { return g_cold_ok; }
void mc_extruder_cold_ok_set(bool on) { g_cold_ok = on; }

void mc_extruder_note_refused() { mc_dbg_150_ex_refused = mc_dbg_150_ex_refused + 1U; }

}  // namespace mc
