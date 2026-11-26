// رابط هشدارها و پیام‌های سیستم (بند ۱۰.۱۳).
#include "alert.h"
#include "hal_buzzer.h"
#include "hal_time.h"
#include "host_reply.h"
#include "../ui/ui_msg.h"

extern "C" {
volatile uint32_t mc_dbg_481_alert_force       = 0U;
volatile uint32_t mc_dbg_482_alert_raise_cnt   = 0U;
volatile uint32_t mc_dbg_483_alert_last_level  = 0U;
volatile uint32_t mc_dbg_484_alert_kill_screen = 0U;
volatile uint32_t mc_dbg_493_alert_repeat_drop_cnt = 0U;
volatile uint32_t mc_dbg_496_alert_last_id     = 0U;
volatile uint32_t mc_dbg_497_alert_id_force    = 0U;
}

namespace mc {
namespace {

struct AlertMeta {
    AlertLevel  level;
    const char* title;
    const char* default_detail;
};

static const AlertMeta kAlertMetaTable[17] = {
    { AlertLevel::Info,    "",                            "" },
    { AlertLevel::Error,   "HOMING FAILED",               "HOMING ERROR" },
    { AlertLevel::Error,   "PROBE FAILED",                "PROBE ERROR" },
    { AlertLevel::Error,   "FILAMENT CHANGE FAILED",      "M600 FAILED" },
    { AlertLevel::Error,   "FILAMENT LOAD FAILED",        "FILAMENT LOAD ERROR" },
    { AlertLevel::Error,   "FILAMENT RUNOUT",             "CHANGE NOT STARTED - CHECK PRINT" },
    { AlertLevel::Error,   "PID AUTOTUNE FAILED",         "AUTOTUNE ERROR" },
    { AlertLevel::Error,   "SETTINGS SAVE FAILED",        "EEPROM WRITE ERROR" },
    { AlertLevel::Warning, "COLD EXTRUSION BLOCKED",      "HEAT THE HOTEND FIRST" },
    { AlertLevel::Warning, "STARTUP SCREEN FAILED",       "SPLASH ERROR" },
    { AlertLevel::Warning, "SETTINGS NOT LOADED",         "EEPROM ERROR - DEFAULTS USED" },
    { AlertLevel::Warning, "SETTINGS NOT LOADED",         "SAVED DATA INVALID - DEFAULTS USED" },
    { AlertLevel::Warning, "RESTARTED BY WATCHDOG",       "CHECK POWER AND WIRING" },
    { AlertLevel::Warning, "HOTEND TURNED OFF",           "FILAMENT CHANGE WAIT TIMEOUT" },
    { AlertLevel::Warning, "PARK MOVE SKIPPED",           "AXIS NOT HOMED" },
    { AlertLevel::Warning, "SET HOTEND TEMP FIRST",       "FOR M600 / M701 / M702" },
    { AlertLevel::Warning, "FILAMENT NOT DETECTED",       "INSERT FILAMENT FIRST" }
};

static uint32_t g_alert_last_ms[17] = { 0U };
static bool     g_alert_has_been_raised[17] = { false };
static volatile bool g_cold_extrude_alert_flag = false;

}  // namespace

void mc_alert_init() {
    mc_dbg_481_alert_force       = 0U;
    mc_dbg_482_alert_raise_cnt   = 0U;
    mc_dbg_483_alert_last_level  = 0U;
    mc_dbg_484_alert_kill_screen = 0U;
    mc_dbg_493_alert_repeat_drop_cnt = 0U;
    mc_dbg_496_alert_last_id     = 0U;
    mc_dbg_497_alert_id_force    = 0U;

    for (uint8_t i = 0U; i < 17U; ++i) {
        g_alert_last_ms[i] = 0U;
        g_alert_has_been_raised[i] = false;
    }
    g_cold_extrude_alert_flag = false;
}

void mc_alert_cold_extrude_trigger() {
    g_cold_extrude_alert_flag = true;
}

void mc_alert_raise_id(AlertId id, const char* detail, uint32_t code) {
    const uint8_t idx = static_cast<uint8_t>(id);
    if ((idx == 0U) || (idx > 16U)) { return; }

    const uint32_t now = mc_millis();
    if (g_alert_has_been_raised[idx] && (now - g_alert_last_ms[idx] < MC_ALERT_REPEAT_MS)) {
        mc_dbg_493_alert_repeat_drop_cnt = mc_dbg_493_alert_repeat_drop_cnt + 1U;
        return;
    }
    g_alert_has_been_raised[idx] = true;
    g_alert_last_ms[idx] = now;
    mc_dbg_496_alert_last_id = static_cast<uint32_t>(idx);

    const AlertMeta& meta = kAlertMetaTable[idx];
    const char* base_detail = ((detail != nullptr) && (detail[0] != '\0')) ? detail : meta.default_detail;

    if (code != 0U) {
        char dbuf[128];
        uint16_t n = 0U;
        const char* p = base_detail;
        while ((*p != '\0') && (n < 90U)) { dbuf[n++] = *p++; }
        const char* code_p = " (CODE ";
        while (*code_p != '\0') { dbuf[n++] = *code_p++; }
        if (code >= 1000U) { dbuf[n++] = static_cast<char>('0' + (code / 1000U)); }
        if (code >= 100U)  { dbuf[n++] = static_cast<char>('0' + ((code % 1000U) / 100U)); }
        if (code >= 10U)   { dbuf[n++] = static_cast<char>('0' + ((code % 100U) / 10U)); }
        dbuf[n++] = static_cast<char>('0' + (code % 10U));
        dbuf[n++] = ')';
        dbuf[n] = '\0';
        mc_alert_raise(meta.level, meta.title, dbuf);
    } else {
        mc_alert_raise(meta.level, meta.title, base_detail);
    }
}

void mc_alert_poll() {
    if (g_cold_extrude_alert_flag) {
        g_cold_extrude_alert_flag = false;
        mc_alert_raise_id(AlertId::ColdExtrude);
    }

    if ((mc_dbg_497_alert_id_force >= 1U) && (mc_dbg_497_alert_id_force <= 16U)) {
        const uint32_t force_id = mc_dbg_497_alert_id_force;
        mc_dbg_497_alert_id_force = 0U;
        mc_alert_raise_id(static_cast<AlertId>(force_id), nullptr, 0U);
    }

    if (mc_dbg_481_alert_force == 0U) { return; }

    const uint32_t force_val = mc_dbg_481_alert_force;
    mc_dbg_481_alert_force = 0U;

    if ((force_val >= 1U) && (force_val <= 3U)) {
        const AlertLevel lvl = static_cast<AlertLevel>(force_val);
        const char* txt = (lvl == AlertLevel::Info) ? "SAMPLE INFO ALERT" :
                          ((lvl == AlertLevel::Warning) ? "SAMPLE WARNING ALERT" : "SAMPLE ERROR ALERT");
        const char* dtl = "TESTED VIA MC_DBG_481_ALERT_FORCE";
        mc_alert_raise(lvl, txt, dtl);
    }
}

void mc_alert_raise(AlertLevel level, const char* text, const char* detail) {
    if (text == nullptr) { return; }

    mc_dbg_482_alert_raise_cnt  = mc_dbg_482_alert_raise_cnt + 1U;
    mc_dbg_483_alert_last_level = static_cast<uint32_t>(level);

    // ۱) بازر غیرمسدودکننده
    if (level == AlertLevel::Error) {
        mc_buzzer_click(MC_ALERT_BEEP_MS * 2U);
    } else {
        mc_buzzer_click(MC_ALERT_BEEP_MS);
    }

    // ۲) پیام روی نمایشگر (از طریق ماژول ui_msg)
    MsgType ui_type = MsgType::Info;
    if (level == AlertLevel::Warning) { ui_type = MsgType::Warning; }
    else if (level == AlertLevel::Error) { ui_type = MsgType::Error; }

    mc_ui_msg_show(ui_type, text, detail);

    // ۳) پاسخ/اعلان سریال به هاست
    if (level == AlertLevel::Error) {
        mc_reply_error(text);
    } else if (level == AlertLevel::Warning) {
        char buf[128];
        uint16_t n = 0U;
        const char* p = "echo:Warning: ";
        while (*p != '\0') { buf[n++] = *p++; }
        p = text;
        while ((*p != '\0') && (n < 120U)) { buf[n++] = *p++; }
        buf[n] = '\0';
        mc_reply_line(buf);
    } else {
        char buf[128];
        uint16_t n = 0U;
        const char* p = "echo:Info: ";
        while (*p != '\0') { buf[n++] = *p++; }
        p = text;
        while ((*p != '\0') && (n < 120U)) { buf[n++] = *p++; }
        buf[n] = '\0';
        mc_reply_line(buf);
    }
}

}  // namespace mc

