// درایور سنسور فیلامنت با فیلتر ضدلرزش/debounce و تشخیص اتمام (بند ۸.۱۶.۴ و ۹.۳).
// هیچ هدر ST اینجا include نمی‌شود. فقط از hal_gpio.h و hal_time.h استفاده می‌کند.
#include "filament_sensor.h"
#include "../hal/hal_gpio.h"
#include "../hal/hal_time.h"
#include "../config/mc_config.h"

extern "C" {
volatile uint32_t mc_dbg_406_fil_raw       = 0;
volatile uint32_t mc_dbg_407_fil_empty     = 0;
volatile uint32_t mc_dbg_408_fil_edges     = 0;
volatile uint32_t mc_dbg_456_fil_run_en   = 0;
volatile uint32_t mc_dbg_457_fil_run_edges = 0;
}

namespace mc {
namespace {

bool     g_inited         = false;
bool     g_raw_level      = false;   // سطح خام پین در دوره قبل
bool     g_stable_level   = false;   // سطح پایدار تأییدشده بعد از debounce
uint32_t g_last_change_ms = 0U;      // زمان آخرین تغییر سطح خام پین
uint32_t g_edge_cnt       = 0U;      // تعداد دفعات تغییر سطح پایدار
bool     g_runout_pending = false;   // پرچم رویداد در انتظار اتمام فیلامنت

}  // namespace

void mc_filament_init() {
    PinMode mode = PinMode::InputPullUp;
#if defined(MC_UI_FIL_PULL_MODE)
#if (MC_UI_FIL_PULL_MODE == 0)
    mode = PinMode::Input;
#elif (MC_UI_FIL_PULL_MODE == 2)
    mode = PinMode::InputPullDown;
#else
    mode = PinMode::InputPullUp;
#endif
#endif

    mc_gpio_set_mode(PinId::FilamentSensor, mode);

    const bool raw = mc_gpio_read(PinId::FilamentSensor);
    g_raw_level      = raw;
    g_stable_level   = raw;
    g_last_change_ms = mc_millis();
    g_edge_cnt       = 0U;
    g_runout_pending = false;   // هنگام init رویدادی ثبت نشود (بند ۹.۳)
    g_inited         = true;

    mc_dbg_456_fil_run_en   = MC_FILRUN_ENABLE_DEFAULT;
    mc_dbg_457_fil_run_edges = 0U;

    const int stable_int = raw ? 1 : 0;
    const bool is_empty  = (stable_int == MC_UI_FIL_EMPTY_LEVEL);

    mc_dbg_406_fil_raw   = raw ? 1U : 0U;
    mc_dbg_407_fil_empty = is_empty ? 1U : 0U;
    mc_dbg_408_fil_edges = 0U;
}

void mc_filament_poll() {
    if (!g_inited) { return; }

    const bool raw = mc_gpio_read(PinId::FilamentSensor);
    const uint32_t now_ms = mc_millis();

    if (raw != g_raw_level) {
        g_raw_level      = raw;
        g_last_change_ms = now_ms;
    } else {
        if ((now_ms - g_last_change_ms) >= static_cast<uint32_t>(MC_UI_FIL_DEBOUNCE_MS)) {
            if (raw != g_stable_level) {
                const int prev_int  = g_stable_level ? 1 : 0;
                const int new_int   = raw ? 1 : 0;
                const bool was_empty = (prev_int == MC_UI_FIL_EMPTY_LEVEL);
                const bool is_empty  = (new_int == MC_UI_FIL_EMPTY_LEVEL);

                g_stable_level = raw;
                g_edge_cnt     = g_edge_cnt + 1U;

                // ثبت لحظه اتمام: تغییر از «فیلامنت هست» به «فیلامنت تمام شده»
                if (!was_empty && is_empty) {
                    mc_dbg_457_fil_run_edges = mc_dbg_457_fil_run_edges + 1U;
                    if (mc_filament_runout_enabled()) {
                        g_runout_pending = true;
                    }
                }
            }
        }
    }

    const int stable_int = g_stable_level ? 1 : 0;
    const bool is_empty  = (stable_int == MC_UI_FIL_EMPTY_LEVEL);

    mc_dbg_406_fil_raw   = raw ? 1U : 0U;
    mc_dbg_407_fil_empty = is_empty ? 1U : 0U;
    mc_dbg_408_fil_edges = g_edge_cnt;
}

bool mc_filament_empty() {
    const int stable_int = g_stable_level ? 1 : 0;
    return (stable_int == MC_UI_FIL_EMPTY_LEVEL);
}

bool mc_filament_runout_enabled() {
    return (mc_dbg_456_fil_run_en != 0U);
}

bool mc_filament_runout_take() {
    if (g_runout_pending) {
        g_runout_pending = false;
        return true;
    }
    return false;
}

}  // namespace mc
