// واکنش خودکار به اتمام فیلامنت (بند ۹.۴). هیچ هدر ST اینجا include نمی‌شود.
#include "filament_runout.h"
#include "../config/mc_config.h"
#include "filament_sensor.h"
#include "filament_change.h"
#include "filament_load.h"
#include "print_state.h"
#include "command_queue.h"
#include "gcode_parser.h"
#include "../gcode/gcode_dispatch.h"
#include "error.h"
#include "log.h"
#include "host_reply.h"
#include "hal_buzzer.h"
#include "alert.h"

extern "C" {
volatile uint32_t mc_dbg_458_frun_last     = 0;  // 0 هیچ، 1 M600 در صف، 2 رد (چاپ نیست)، 3 رد (تعویض/بارگذاری فعال)، 4 رد (killed)، 5 صف پر بود
volatile uint32_t mc_dbg_459_frun_trig_cnt = 0;  // تعداد M600های ثبت‌شده با runout
volatile uint32_t mc_dbg_460_frun_ign_cnt  = 0;  // تعداد رویدادهای ردشده
volatile uint32_t mc_dbg_463_frun_caused   = 0;  // 1 = M600 جاری توسط runout ایجاد شده
}

namespace mc {
namespace {

bool g_retry_push     = false;
bool g_caused_pending = false;
bool g_filchg_started = false;

}  // namespace

void mc_filrun_init() {
    g_retry_push     = false;
    g_caused_pending = false;
    g_filchg_started = false;

    mc_dbg_458_frun_last     = 0U;
    mc_dbg_459_frun_trig_cnt = 0U;
    mc_dbg_460_frun_ign_cnt  = 0U;
    mc_dbg_463_frun_caused   = 0U;
}

bool mc_filrun_caused() {
    return (mc_dbg_463_frun_caused != 0U);
}

void mc_filrun_abort() {
    g_retry_push     = false;
    g_caused_pending = false;
    g_filchg_started = false;
    mc_dbg_463_frun_caused = 0U;
}

void mc_filrun_poll() {
    // ۱) پایش متغیر mc_dbg_463_frun_caused و شروع/پایان M600
    if (g_caused_pending) {
        if (mc_filchg_active()) {
            g_filchg_started = true;
        } else if (g_filchg_started) {
            // M600 تمام شد (موفق یا لغو شد)
            g_caused_pending = false;
            g_filchg_started = false;
            mc_dbg_463_frun_caused = 0U;
        } else {
            // هنوز M600 شروع نشده است؛ اگر صف خالی شد و M600 اجرا نشد (رد شد)
            if (mc_cmdq_empty() && !mc_dispatch_busy() && !mc_filchg_active()) {
                g_caused_pending = false;
                mc_dbg_463_frun_caused = 0U;
                mc_log(LogLevel::Error, "runout M600 refused");
                mc_alert_raise_id(AlertId::RunoutRefused);
            }
        }
    }

    // ۲) بررسی رویداد اتمام فیلامنت
    bool has_event = false;
    if (g_retry_push) {
        has_event = true;
    } else if (mc_filament_runout_enabled() && mc_filament_runout_take()) {
        has_event = true;
    }

    if (!has_event) { return; }

    // ارزیابی شرایط
    if (mc_is_killed()) {
        g_retry_push = false;
        mc_dbg_458_frun_last = 4U;
        mc_dbg_460_frun_ign_cnt = mc_dbg_460_frun_ign_cnt + 1U;
        return;
    }

    if (mc_filchg_active() || mc_fload_active() || g_caused_pending) {
        g_retry_push = false;
        mc_dbg_458_frun_last = 3U;
        mc_dbg_460_frun_ign_cnt = mc_dbg_460_frun_ign_cnt + 1U;
        return;
    }

    if (!mc_print_active()) {
        g_retry_push = false;
        mc_dbg_458_frun_last = 2U;
        mc_dbg_460_frun_ign_cnt = mc_dbg_460_frun_ign_cnt + 1U;
        mc_log(LogLevel::Info, "Filament runout ignored, not printing");
        return;
    }

    // ساخت دستور M600 و افزودن به صف
    GcodeCommand cmd = {};
    if (mc_gcode_parse("M600", cmd) != GcodeResult::Ok) {
        g_retry_push = false;
        mc_dbg_458_frun_last = 4U;
        mc_dbg_460_frun_ign_cnt = mc_dbg_460_frun_ign_cnt + 1U;
        return;
    }

    if (!mc_cmdq_push(cmd)) {
        // صف پر است، در دور بعد دوباره تلاش می‌شود
        mc_dbg_458_frun_last = 5U;
        g_retry_push = true;
        return;
    }

    // افزودن موفق به صف
    g_retry_push     = false;
    g_caused_pending = true;
    g_filchg_started = false;

    mc_dbg_463_frun_caused   = 1U;
    mc_dbg_458_frun_last     = 1U;
    mc_dbg_459_frun_trig_cnt = mc_dbg_459_frun_trig_cnt + 1U;

    mc_reply_line("echo:Filament runout detected, pausing print");
    mc_log(LogLevel::Warn, "Filament runout detected, M600 queued");
    mc_buzzer_click(MC_FILRUN_ALERT_BEEP_MS);
}

}  // namespace mc
