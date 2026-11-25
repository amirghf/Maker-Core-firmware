// بررسی اندستاپ حین چاپ (بند ۱۰.۸).
#include "endstop_print.h"
#include "endstop.h"
#include "print_state.h"
#include "homing.h"
#include "probe.h"
#include "quick_stop.h"
#include "error.h"
#include "alert.h"

extern "C" {
volatile uint32_t mc_dbg_475_esp_enable    = MC_ENDSTOP_PRINT_CHECK_DEFAULT;
volatile uint32_t mc_dbg_476_esp_trip_cnt  = 0U;
volatile uint32_t mc_dbg_477_esp_last_mask = 0U;
}

namespace mc {

void mc_endstop_print_init() {
    mc_dbg_475_esp_enable    = MC_ENDSTOP_PRINT_CHECK_DEFAULT;
    mc_dbg_476_esp_trip_cnt  = 0U;
    mc_dbg_477_esp_last_mask = 0U;
}

void mc_endstop_print_poll() {
    if (mc_dbg_475_esp_enable == 0U) { return; }
    if (!mc_print_active()) { return; }
    if (mc_homing_busy() || mc_probe_busy()) { return; }

    uint32_t mask = 0U;
    for (uint8_t i = 0U; i < 3U; ++i) {
        if (mc_endstop_triggered(i)) {
            mask |= (1U << i);
        }
    }

    if (mask != 0U) {
        mc_quick_stop();
        mc_dbg_476_esp_trip_cnt  = mc_dbg_476_esp_trip_cnt + 1U;
        mc_dbg_477_esp_last_mask = mask;
        // بند ۱۰.۱۳: استفاده از mc_alert_raise برای اعلام خطا
        mc_alert_raise(AlertLevel::Error, "ENDSTOP TRIGGERED DURING PRINT", "CODE 14 - ENDSTOP DURING PRINT");
    }
}

}  // namespace mc
