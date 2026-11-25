// رابط هشدارها و پیام‌های سیستم (بند ۱۰.۱۳).
#include "alert.h"
#include "hal_buzzer.h"
#include "host_reply.h"
#include "../ui/ui_msg.h"

extern "C" {
volatile uint32_t mc_dbg_481_alert_force       = 0U;
volatile uint32_t mc_dbg_482_alert_raise_cnt   = 0U;
volatile uint32_t mc_dbg_483_alert_last_level  = 0U;
volatile uint32_t mc_dbg_484_alert_kill_screen = 0U;
}

namespace mc {

void mc_alert_init() {
    mc_dbg_481_alert_force       = 0U;
    mc_dbg_482_alert_raise_cnt   = 0U;
    mc_dbg_483_alert_last_level  = 0U;
    mc_dbg_484_alert_kill_screen = 0U;
}

void mc_alert_poll() {
    if (mc_dbg_481_alert_force == 0U) { return; }

    const uint32_t force_val = mc_dbg_481_alert_force;
    mc_dbg_481_alert_force = 0U;

    if (force_val >= 1U && force_val <= 3U) {
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
