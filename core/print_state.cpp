// پایش وضعیت «در حال چاپ» (بند ۹.۴). هیچ هدر ST اینجا include نمی‌شود.
#include "print_state.h"
#include "../config/mc_config.h"
#include "heater_ex.h"
#include "hal_time.h"

extern "C" {
volatile uint32_t mc_dbg_461_frun_print_act  = 0;
volatile uint32_t mc_dbg_462_frun_fake_print = 0;
}

namespace mc {
namespace {

uint32_t g_last_extrude_ms = 0U;

}  // namespace

void mc_print_init() {
    g_last_extrude_ms          = 0U;
    mc_dbg_461_frun_print_act  = 0U;
    mc_dbg_462_frun_fake_print = 0U;
}

void mc_print_poll() {
    (void)mc_print_active();
}

void mc_print_note_extrusion() {
    g_last_extrude_ms = mc_millis();
}

bool mc_print_active() {
    bool active = false;
    if (mc_dbg_462_frun_fake_print != 0U) {
        active = true;
    } else {
        if ((g_last_extrude_ms > 0U) &&
            !mc_time_elapsed_ms(g_last_extrude_ms, static_cast<uint32_t>(MC_FILRUN_PRINT_WINDOW_S) * 1000U) &&
            (mc_heater_ex_target_x10() >= (MC_EXTRUDE_MIN_TEMP_C * 10))) {
            active = true;
        }
    }
    mc_dbg_461_frun_print_act = active ? 1U : 0U;
    return active;
}

}  // namespace mc
