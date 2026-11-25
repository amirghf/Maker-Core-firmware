// G92: موقعیت فعلی را بدون حرکت عوض می‌کند. مثل G92 X10 Y0 E0
// فقط محورهایی که در خط آمده‌اند عوض می‌شوند. بدون پارامتر: هیچ کاری نمی‌کند.
// اگر یکی از مقدارها نامعتبر باشد (بدون عدد یا قدر مطلق بیشتر از ۹۹۹۹۹)، هیچ‌کدام اعمال نمی‌شود و Error برمی‌گردد.
// تا وقتی بلوک حرکت در صف یا در اجراست صبر می‌کند (Busy).
#include "gcode_handlers.h"
#include "machine_state.h"
#include "motion_modes.h"
#include "motion_queue.h"
#include "endstop_guard.h"   // بند ۶.۲
#include "soft_limits.h"    // بند ۱۰.۶
#include "homing.h"         // بند ۱۰.۶
#include "host_reply.h"     // بند ۱۰.۶

namespace mc {

GcodeStatus mc_gcode_g92(const GcodeCommand& cmd, bool first_call) {
    (void)first_call;
    static const char kLetters[4] = { 'X', 'Y', 'Z', 'E' };
    bool  has[4];
    float val[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    for (uint8_t i = 0U; i < 4U; ++i) {
        has[i] = mc_gcode_has(cmd, kLetters[i]);
        if (!has[i]) { continue; }
        if (!mc_gcode_has_value(cmd, kLetters[i])) {
            mc_mode_note_refused();
            return GcodeStatus::Error;
        }
        val[i] = mc_gcode_value(cmd, kLetters[i], 0.0f);
        if (!((val[i] >= -kModeMaxPositionMm) && (val[i] <= kModeMaxPositionMm))) {   // NaN هم رد می‌شود
            mc_mode_note_refused();
            return GcodeStatus::Error;
        }
    }

    if (mc_mq_busy()) { return GcodeStatus::Busy; }

    // بند ۱۰.۶: G92 روی محور هوم‌شده: هر مقدار تازه‌ای که موقعیت فعلی را بیرون از محدوده بگذارد رد می‌شود
    if (mc_softlimit_enabled_get()) {
        const uint8_t homed_mask = mc_homing_homed_mask();
        for (uint8_t i = 0U; i < 3U; ++i) {
            if (has[i] && ((homed_mask & (1U << i)) != 0U)) {
                if (!mc_softlimit_allows(i, val[i])) {
                    mc_softlimit_note_homed_refused(3U);  // 3 = G92
                    mc_softlimit_note_refused();
                    mc_mode_note_refused();
                    mc_reply_error("G92 position out of bounds for homed axis");
                    return GcodeStatus::DoneReplied;
                }
            }
        }
    }

    bool any = false;
    for (uint8_t i = 0U; i < 4U; ++i) {
        if (has[i]) {
            mc_position_set(i, val[i]);
            any = true;
        }
    }
    if (any) {
        mc_mode_note_applied();
        mc_eguard_snapshot();   // بند ۶.۲: صف خالی است (بالاتر Busy برگردانده می‌شد)
    }
    return GcodeStatus::Done;
}

}  // namespace mc
