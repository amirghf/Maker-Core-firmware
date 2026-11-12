#include "gcode_dispatch.h"
#include "gcode_handlers.h"
#include "command_queue.h"
#include "log.h"

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_disp_letter     = 0;   // کد حرف آخرین دستور برداشته‌شده (۷۱ = G ، ۷۷ = M ، ۸۴ = T)
volatile uint32_t mc_dbg_disp_number     = 0;   // شماره آخرین دستور برداشته‌شده
volatile int32_t  mc_dbg_disp_sub        = -1;  // زیرکد آخرین دستور (یا -۱)
volatile uint32_t mc_dbg_disp_taken      = 0;   // تعداد دستورهای برداشته‌شده از صف
volatile uint32_t mc_dbg_disp_done       = 0;   // تعداد دستورهایی که با Done تمام شدند
volatile uint32_t mc_dbg_disp_unknown    = 0;   // تعداد دستورهای ناشناس (در جدول نبودند)
volatile uint32_t mc_dbg_disp_errors     = 0;   // تعداد دستورهایی که Error برگرداندند
volatile uint32_t mc_dbg_disp_busy       = 0;   // ۱ = الان یک دستور در حال اجراست
}

namespace mc {
namespace {

GcodeCommand       g_cmd;                 // دستور در حال اجرا (حدود ۲۳۶ بایت، ثابت)
const GcodeEntry*  g_entry  = nullptr;    // ردیف جدول برای دستور در حال اجرا
bool               g_active = false;      // یک دستور در حال اجراست
bool               g_first  = true;       // بار اول صدا زدن تابع این دستور

}  // namespace

bool mc_dispatch_busy() {
    return g_active;
}

void mc_dispatch_poll() {
    if (!g_active) {
        if (!mc_cmdq_pop(g_cmd)) { return; }
        mc_dbg_disp_taken  = mc_dbg_disp_taken + 1U;
        mc_dbg_disp_letter = static_cast<uint32_t>(static_cast<uint8_t>(g_cmd.letter));
        mc_dbg_disp_number = g_cmd.number;
        mc_dbg_disp_sub    = g_cmd.sub;

        g_entry = mc_gcode_find(g_cmd.letter, g_cmd.number, g_cmd.sub);
        if (g_entry == nullptr) {
            mc_dbg_disp_unknown = mc_dbg_disp_unknown + 1U;
            if (g_cmd.letter == 'G') {
                mc_log_value(LogLevel::Warn, "unknown G command", g_cmd.number);
            } else if (g_cmd.letter == 'M') {
                mc_log_value(LogLevel::Warn, "unknown M command", g_cmd.number);
            } else {
                mc_log_value(LogLevel::Warn, "unknown T command", g_cmd.number);
            }
            return;
        }
        g_active = true;
        g_first  = true;
        mc_dbg_disp_busy = 1U;
    }

    const GcodeStatus s = g_entry->handler(g_cmd, g_first);
    g_first = false;
    if (s == GcodeStatus::Busy) { return; }

    g_active = false;
    mc_dbg_disp_busy = 0U;
    if (s == GcodeStatus::Error) {
        mc_dbg_disp_errors = mc_dbg_disp_errors + 1U;
        mc_log_value(LogLevel::Error, "gcode command failed", g_cmd.number);
    } else {
        mc_dbg_disp_done = mc_dbg_disp_done + 1U;
    }
}

}  // namespace mc
