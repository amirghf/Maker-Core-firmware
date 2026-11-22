// M501: بازیابی تنظیمات از EEPROM و اعمال در RAM (بند ۷.۳).
#include "gcode_handlers.h"
#include "settings_store.h"
#include "settings_data.h"
#include "settings_boot.h"
#include "motion_queue.h"
#include "host_reply.h"
#include "hal_time.h"

namespace mc {
namespace {

enum class LoadState : uint8_t { Idle = 0, PendingStart = 1, WaitingLoad = 2 };

LoadState g_state   = LoadState::Idle;
uint32_t  g_t_start = 0U;
uint8_t   g_buf[kSettingsPayloadMax];

void reset_state() {
    g_state = LoadState::Idle;
}

}  // namespace

GcodeStatus mc_gcode_m501(const GcodeCommand& cmd, bool first_call) {
    (void)cmd;

    if (mc_cfg_boot_busy()) { return GcodeStatus::Busy; }
    if (mc_mq_busy()) { return GcodeStatus::Busy; }


    if (first_call) {
        g_t_start = mc_millis();
        if (!mc_settings_load_start(g_buf, static_cast<uint16_t>(sizeof(g_buf)))) {
            g_state = LoadState::PendingStart;
            return GcodeStatus::Busy;
        }
        g_state = LoadState::WaitingLoad;
        return GcodeStatus::Busy;
    }

    if (g_state == LoadState::Idle) {
        return GcodeStatus::Error;
    }

    if (mc_time_elapsed_ms(g_t_start, MC_CFG_CMD_TIMEOUT_MS)) {
        mc_cfg_note(501U, 6U, 0U, mc_settings_result());
        reset_state();
        return GcodeStatus::Error;
    }

    if (g_state == LoadState::PendingStart) {
        if (!mc_settings_load_start(g_buf, static_cast<uint16_t>(sizeof(g_buf)))) {
            return GcodeStatus::Busy;
        }
        g_state = LoadState::WaitingLoad;
        return GcodeStatus::Busy;
    }

    const SettingsState st = mc_settings_state();
    if (st == SettingsState::Busy) {
        return GcodeStatus::Busy;
    }

    const uint8_t store_res = mc_settings_result();
    if (st == SettingsState::Done) {
        const uint16_t loaded_len = mc_settings_loaded_len();
        const uint8_t chk = mc_cfg_check(g_buf, loaded_len);
        if (chk == 0U) {
            (void)mc_cfg_apply(g_buf, loaded_len);
            mc_cfg_note(501U, 1U, loaded_len, store_res);
            mc_reply_line("Settings loaded");
            reset_state();
            return GcodeStatus::Done;
        }
        mc_cfg_note(501U, chk, loaded_len, store_res);
        reset_state();
        return GcodeStatus::Error;
    }

    // st == SettingsState::Error
    const uint8_t res_code = (store_res == kSetResNoValid) ? 3U : 2U;
    mc_cfg_note(501U, res_code, 0U, store_res);
    reset_state();
    return GcodeStatus::Error;
}

}  // namespace mc
