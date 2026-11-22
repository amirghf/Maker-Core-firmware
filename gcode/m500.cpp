// M500: ذخیره تنظیمات فعلی روی EEPROM (بند ۷.۳).
#include "gcode_handlers.h"
#include "settings_store.h"
#include "settings_data.h"
#include "host_reply.h"
#include "hal_time.h"

namespace mc {
namespace {

enum class SaveState : uint8_t { Idle = 0, PendingStart = 1, WaitingStore = 2 };

SaveState g_state   = SaveState::Idle;
uint32_t  g_t_start = 0U;
uint8_t   g_buf[kSettingsPayloadMax];
uint16_t  g_len   = 0U;

void reset_state() {
    g_state = SaveState::Idle;
}

}  // namespace

GcodeStatus mc_gcode_m500(const GcodeCommand& cmd, bool first_call) {
    (void)cmd;

    if (first_call) {
        g_t_start = mc_millis();
        g_len = mc_cfg_collect(g_buf, static_cast<uint16_t>(sizeof(g_buf)));
        if (g_len == 0U) {
            mc_cfg_note(500U, 5U, 0U, 0U);
            reset_state();
            return GcodeStatus::Error;
        }
        if (!mc_settings_save_start(g_buf, g_len)) {
            g_state = SaveState::PendingStart;
            return GcodeStatus::Busy;
        }
        g_state = SaveState::WaitingStore;
        return GcodeStatus::Busy;
    }

    if (g_state == SaveState::Idle) {
        return GcodeStatus::Error;
    }

    if (mc_time_elapsed_ms(g_t_start, MC_CFG_CMD_TIMEOUT_MS)) {
        mc_cfg_note(500U, 6U, g_len, mc_settings_result());
        reset_state();
        return GcodeStatus::Error;
    }

    if (g_state == SaveState::PendingStart) {
        if (!mc_settings_save_start(g_buf, g_len)) {
            return GcodeStatus::Busy;
        }
        g_state = SaveState::WaitingStore;
        return GcodeStatus::Busy;
    }

    const SettingsState st = mc_settings_state();
    if (st == SettingsState::Busy) {
        return GcodeStatus::Busy;
    }

    const uint8_t store_res = mc_settings_result();
    if (st == SettingsState::Done) {
        mc_cfg_note(500U, 1U, g_len, store_res);
        mc_reply_line("Settings stored");
        reset_state();
        return GcodeStatus::Done;
    }

    // st == SettingsState::Error
    mc_cfg_note(500U, 2U, g_len, store_res);
    reset_state();
    return GcodeStatus::Error;
}

}  // namespace mc
