// تشخیص بند ۶.۴. فقط خواندن و کپی می‌کند و به هیچ سخت‌افزاری دست نمی‌زند.
#include "boot_diag.h"
#include <stdint.h>

// متغیرهای موجود در فایل‌های دیگر (فقط خوانده می‌شوند).
extern "C" {
extern volatile uint32_t mc_dbg_152_sl_enabled;
extern volatile uint32_t mc_dbg_163_sl_refused;
extern volatile uint32_t mc_dbg_215_m119_runs;
}

// متغیرهای debug؛ در CubeIDE با Live Expressions دیده می‌شوند. شماره‌ها دائمی‌اند.
extern "C" {
volatile uint32_t mc_dbg_217_diag_addr_m119   = 0;  // خواندنی: آدرس واقعی mc_dbg_215_m119_runs که برنامه روی برد می‌بیند
volatile uint32_t mc_dbg_218_diag_addr_sl_en  = 0;  // خواندنی: آدرس واقعی mc_dbg_152_sl_enabled که برنامه روی برد می‌بیند
volatile uint32_t mc_dbg_219_diag_boot_sl_en  = 0;  // خواندنی: مقدار mc_dbg_152_sl_enabled درست بعد از همه init ها
volatile uint32_t mc_dbg_220_diag_boot_sl_ref = 0;  // خواندنی: مقدار mc_dbg_163_sl_refused درست بعد از همه init ها
volatile uint32_t mc_dbg_221_diag_build_id    = 0;  // خواندنی: شناسه زمان کامپایل همین فایل (روز*1000000 + ساعت دقیقه ثانیه)
volatile uint32_t mc_dbg_222_diag_poll_count  = 0;  // خواندنی: تعداد دفعات اجرای poll (هر 100 میلی‌ثانیه یکی)
volatile uint32_t mc_dbg_223_diag_live_sl_en  = 0;  // خواندنی: مقداری که کد همین الان از mc_dbg_152_sl_enabled می‌خواند
volatile uint32_t mc_dbg_224_diag_live_sl_ref = 0;  // خواندنی: مقداری که کد همین الان از mc_dbg_163_sl_refused می‌خواند
volatile uint32_t mc_dbg_225_diag_live_m119   = 0;  // خواندنی: مقداری که کد همین الان از mc_dbg_215_m119_runs می‌خواند
}

namespace mc {
namespace {

constexpr uint32_t digit_of(char c) {
    return (c >= '0' && c <= '9') ? static_cast<uint32_t>(c - '0') : 0U;
}

// __DATE__ شکل "Nov 19 2026" دارد (روز در جای 4 و 5) و __TIME__ شکل "18:19:43".
constexpr uint32_t kBuildId =
    (digit_of(__DATE__[4]) * 10U + digit_of(__DATE__[5])) * 1000000U +
    (digit_of(__TIME__[0]) * 10U + digit_of(__TIME__[1])) * 10000U +
    (digit_of(__TIME__[3]) * 10U + digit_of(__TIME__[4])) * 100U +
    (digit_of(__TIME__[6]) * 10U + digit_of(__TIME__[7]));

}  // namespace

void mc_diag_init() {
    mc_dbg_217_diag_addr_m119   = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&mc_dbg_215_m119_runs));
    mc_dbg_218_diag_addr_sl_en  = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&mc_dbg_152_sl_enabled));
    mc_dbg_219_diag_boot_sl_en  = mc_dbg_152_sl_enabled;
    mc_dbg_220_diag_boot_sl_ref = mc_dbg_163_sl_refused;
    mc_dbg_221_diag_build_id    = kBuildId;
}

void mc_diag_poll() {
    mc_dbg_222_diag_poll_count  = mc_dbg_222_diag_poll_count + 1U;
    mc_dbg_223_diag_live_sl_en  = mc_dbg_152_sl_enabled;
    mc_dbg_224_diag_live_sl_ref = mc_dbg_163_sl_refused;
    mc_dbg_225_diag_live_m119   = mc_dbg_215_m119_runs;
}

}  // namespace mc
