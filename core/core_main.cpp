// حلقه اصلی هسته MakerCore: زمان‌بند و دو وظیفه نمونه برای تست روی برد.
// هیچ هدر ST اینجا include نمی‌شود.
#include "core_main.h"
#include "scheduler.h"
#include "hal_watchdog.h"
#include "error.h"
#include "log.h"
#include "hal_time.h"
#include "hal_display.h"
#include "gfx.h"
#include "hal_touch.h"
#include "touch.h"
#include "hal_flash.h"
#include "splash.h"
#include "line_reader.h"
#include "gcode_parser.h"
#include "line_check.h"
#include "command_queue.h"
#include "gcode_dispatch.h"
#include "host_reply.h"
#include "adc_sampler.h"
#include "thermistor.h"
#include "output_pwm.h"
#include "heater_ex.h"
#include "heater_bed.h"
#include "temp_limits.h"
#include "temp_protect.h"
#include "temp_report.h"
#include "stepper_out.h"
#include "step_gen.h"
#include "axis_move.h"
#include "motion_queue.h"
#include "planner.h"
#include "motion_modes.h"
#include "extruder.h"
#include "soft_limits.h"
#include "motion_limits.h"
#include "motion_cmd.h"
#include "endstop.h"
#include "endstop_guard.h"
#include "homing.h"
#include "bltouch.h"
#include "probe.h"
#include "filament_sensor.h"
#include "filament_change.h"
#include "filament_load.h"
#include "filament_runout.h"
#include "print_state.h"
#include "endstop_print.h"
#include "alert.h"
#include "eeprom_at24.h"
#include "settings_store.h"
#include "settings_boot.h"
#include "encoder.h"
#include "touch_input.h"
#include "touch_cal_store.h"
#include "disp_settings.h"
#include "../ui/ui_home.h"


// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_sched_100ms          = 0;  // هر ۱۰۰ میلی‌ثانیه یکی زیاد می‌شود
volatile uint32_t mc_dbg_25_sched_1s          = 0;  // هر ۱ ثانیه یکی زیاد می‌شود
volatile uint32_t mc_dbg_26_sched_max_task_us = 0;  // طولانی‌ترین اجرای یک وظیفه (میکروثانیه)
volatile uint32_t mc_dbg_184_sched_count       = 0;  // خواندنی: تعداد وظیفه‌های ثبت‌شده در زمان‌بند (بند ۵.۱۱)
volatile uint32_t mc_dbg_kill_request         = 0;  // برای تست: نوشتن عدد ۱ در این متغیر، kill را اجرا می‌کند
volatile uint32_t mc_dbg_log_test_request  = 0;  // برای تست: نوشتن عدد ۱ در این متغیر، چند خط آزمایشی لاگ می‌کند
volatile uint32_t mc_dbg_uptime_s          = 0;  // ثانیه از روشن‌شدن برد
volatile uint32_t mc_dbg_heartbeat_count   = 0;  // تعداد دفعاتی که لاگ ۶۰ ثانیه‌ای اجرا شده
volatile uint32_t mc_dbg_tft_test_request  = 0;  // برای تست: نوشتن عدد ۱ در این متغیر، تست نمایشگر را اجرا می‌کند
volatile uint32_t mc_dbg_tft_step          = 0;  // مرحله جاری تست نمایشگر (۰ = بیکار)
volatile uint32_t mc_dbg_gfx_test_request  = 0;  // برای تست: نوشتن عدد ۱ در این متغیر، تست گرافیک را اجرا می‌کند
volatile uint32_t mc_dbg_gfx_fill_ms       = 0;  // زمان پر کردن کل صفحه (میلی‌ثانیه)
volatile uint32_t mc_dbg_gfx_text_ms       = 0;  // زمان نوشتن تمام تست‌های متن (میلی‌ثانیه)
volatile uint32_t mc_dbg_flash_test_request = 0;  // برای تست: نوشتن عدد ۱ در این متغیر، تست فلش را اجرا می‌کند

volatile uint32_t mc_dbg_flash_test         = 0;  // نتیجه تست: ۰ = اجرا نشده، ۱ = پاس، بقیه کد خطا
volatile uint32_t mc_dbg_flash_read_ms      = 0;  // زمان خواندن ۶۴ کیلوبایت (میلی‌ثانیه)
volatile uint32_t mc_dbg_flash_erase_ms     = 0;  // زمان پاک‌کردن یک سکتور (میلی‌ثانیه)
volatile uint32_t mc_dbg_flash_write_ms     = 0;  // زمان نوشتن ۳۰۰ بایت (میلی‌ثانیه)
volatile uint32_t mc_dbg_splash_request     = 0;  // برای تست: نوشتن عدد ۱ در این متغیر، اسپلش را دوباره نشان می‌دهد
volatile uint32_t mc_dbg_splash_result      = 0;  // نتیجه: ۰ = اجرا نشده، ۱ = موفق، ۲ تا ۵ = کد خطا (توضیح در splash.h)
volatile uint32_t mc_dbg_splash_ms          = 0;  // زمان کل نمایش اسپلش (میلی‌ثانیه)
volatile uint32_t mc_dbg_gc_result          = 0;  // نتیجه تجزیه: ۰ = درست، ۱ = خالی یا فقط کامنت، ۲ = کلمه دستور خراب، ۳ = پارامتر خراب، ۴ = پارامتر زیاد
volatile uint32_t mc_dbg_gc_letter          = 0;  // کد حرف دستور (۷۱ = G ، ۷۷ = M ، ۸۴ = T ، ۰ = هیچ)
volatile uint32_t mc_dbg_gc_number          = 0;  // شماره دستور
volatile int32_t  mc_dbg_gc_sub             = -1; // زیرکد (G38.2 ← ۲) یا -۱
volatile uint32_t mc_dbg_gc_params          = 0;  // تعداد پارامترها
volatile float    mc_dbg_gc_x               = 0;  // مقدار X (اگر نبود یا بدون عدد بود: -۹۹۹)
volatile float    mc_dbg_gc_y               = 0;  // مقدار Y (همین‌طور)
volatile float    mc_dbg_gc_z               = 0;  // مقدار Z (همین‌طور)
volatile float    mc_dbg_gc_e               = 0;  // مقدار E (همین‌طور)
volatile float    mc_dbg_gc_f               = 0;  // مقدار F (همین‌طور)
volatile float    mc_dbg_gc_s               = 0;  // مقدار S (همین‌طور)
volatile uint32_t mc_dbg_cq_pause           = 0;  // برای تست: ۱ = برداشتن از صف متوقف می‌شود (صف پر می‌شود)، ۰ = عادی
volatile uint32_t mc_dbg_cq_full_waits      = 0;  // تعداد خط‌هایی که به‌خاطر پر بودن صف منتظر ماندند (هر خط یک بار)
}

namespace mc {
namespace {

Scheduler g_main;

void task_100ms() {
    mc_dbg_sched_100ms = mc_dbg_sched_100ms + 1U;
}

void task_1s() {
    mc_dbg_25_sched_1s = mc_dbg_25_sched_1s + 1U;
    mc_dbg_26_sched_max_task_us = g_main.max_task_us();
    mc_dbg_184_sched_count = g_main.count();
    mc_dbg_uptime_s = mc_millis() / 1000U;
}

void task_watchdog_feed() {
    mc_watchdog_feed();
}

void task_kill_test() {
    if (mc_dbg_kill_request != 0U) {
        mc_kill(ErrorCode::Test);
    }
}

void task_log_flush() {
    mc_log_flush();
}

void task_log_test() {
    if (mc_dbg_log_test_request != 0U) {
        mc_dbg_log_test_request = 0U;
        mc_log(LogLevel::Error, "log test error");
        mc_log(LogLevel::Warn, "log test warn");
        mc_log(LogLevel::Info, "log test info");
        mc_log(LogLevel::Debug, "log test debug");
        mc_log_value(LogLevel::Info, "log test value", 12345U);
    }
}

void task_heartbeat() {
    // بعد از روشن‌شدن Watchdog، اگر ریست قبلی از Watchdog بوده، فقط یک‌بار هشدار بده
    static bool boot_reported = false;
    if ((!boot_reported) && mc_watchdog_active()) {
        boot_reported = true;
        if (mc_watchdog_was_reset()) {
            mc_log(LogLevel::Warn, "last reset was by watchdog");
        }
    }

    mc_dbg_heartbeat_count = mc_dbg_heartbeat_count + 1U;
    mc_log_value(LogLevel::Info, "uptime_s", mc_millis() / 1000U);
    mc_log_value(LogLevel::Info, "max_task_us", g_main.max_task_us());
}

void task_tft_test() {
    static uint8_t  step    = 0U;   // ۰ = بیکار ، ۱ تا ۶ = مرحله‌های تست
    static uint32_t last_ms = 0U;

    if (step == 0U) {
        if (mc_dbg_tft_test_request == 0U) { return; }
        mc_dbg_tft_test_request = 0U;

        const bool ok = mc_display_init();
        if (!ok) {
            mc_log(LogLevel::Error, "display init failed");
            return;
        }
        mc_log(LogLevel::Info, "display init ok");
        mc_display_fill_screen(kColorBlack);   // اول صفحه سیاه، بعد بک‌لایت روشن
        mc_display_backlight(true);
        last_ms = mc_millis();
        step = 1U;
        mc_dbg_tft_step = step;
        return;
    }

    // بین مرحله‌ها حداقل یک ثانیه صبر کن تا رنگ‌ها دیده شوند
    if (!mc_time_elapsed_ms(last_ms, 1000U)) { return; }

    static const uint16_t kColors[5] = { kColorRed, kColorGreen, kColorBlue, kColorWhite, kColorBlack };
    if (step <= 5U) {
        mc_display_fill_screen(kColors[step - 1U]);
    } else {
        // مرحله ۶: چهار مربع ۶۰ در ۶۰ در گوشه‌ها روی زمینه سیاه
        const uint16_t w = mc_display_width();
        const uint16_t h = mc_display_height();
        mc_display_fill_rect(0U, 0U, 60U, 60U, kColorRed);                                   // بالا چپ: قرمز
        mc_display_fill_rect(static_cast<uint16_t>(w - 60U), 0U, 60U, 60U, kColorGreen);     // بالا راست: سبز
        mc_display_fill_rect(0U, static_cast<uint16_t>(h - 60U), 60U, 60U, kColorBlue);      // پایین چپ: آبی
        mc_display_fill_rect(static_cast<uint16_t>(w - 60U), static_cast<uint16_t>(h - 60U),
                             60U, 60U, kColorWhite);                                         // پایین راست: سفید
    }
    last_ms = mc_millis();
    mc_dbg_tft_step = step;
    step = (step >= 6U) ? 0U : static_cast<uint8_t>(step + 1U);
}

void task_gfx_test() {
    if (mc_dbg_gfx_test_request == 0U) { return; }
    mc_dbg_gfx_test_request = 0U;

    if (!mc_display_init()) {
        mc_log(LogLevel::Error, "display init failed");
        return;
    }
    mc_display_fill_screen(kColorBlack);
    mc_display_backlight(true);

    // زمان پر کردن کل صفحه
    uint32_t t0 = mc_millis();
    mc_display_fill_screen(kColorBlue);
    mc_dbg_gfx_fill_ms = mc_millis() - t0;
    mc_display_fill_screen(kColorBlack);

    // قاب دور صفحه و چند شکل
    t0 = mc_millis();
    mc_gfx_rect(0U, 0U, mc_display_width(), mc_display_height(), kColorWhite);
    mc_gfx_rect(4U, 4U, mc_display_width() - 8U, mc_display_height() - 8U, kColorRed);
    mc_gfx_fill_rect(300U, 40U, 60U, 30U, kColorGreen);
    mc_gfx_hline(300U, 80U, 100U, kColorWhite);
    mc_gfx_vline(410U, 40U, 60U, kColorWhite);
    mc_gfx_pixel(420U, 50U, kColorRed);
    mc_gfx_pixel(421U, 50U, kColorGreen);
    mc_gfx_pixel(422U, 50U, kColorBlue);

    // متن در چهار اندازه
    mc_gfx_text(12U, 12U,  "MakerCore | MW-X1", kColorWhite, kColorBlack, 2U);
    mc_gfx_text(12U, 40U,  "Scale 1: Hello 0123456789", kColorGreen, kColorBlack, 1U);
    mc_gfx_text(12U, 56U,  "Scale 2: Hello 0123", kColorGreen, kColorBlack, 2U);
    mc_gfx_text(12U, 80U,  "Scale 3: Abc 123", kColorBlue, kColorWhite, 3U);
    mc_gfx_text(12U, 112U, "Scale 4: Xyz", kColorRed, kColorBlack, 4U);

    // همه کاراکترهای ASCII در دو سطر
    mc_gfx_text(12U, 160U,
                " !\"#$%&'()*+,-./0123456789:;<=>?\n"
                "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\n"
                "`abcdefghijklmnopqrstuvwxyz{|}~",
                kColorWhite, kColorBlack, 2U);
    mc_dbg_gfx_text_ms = mc_millis() - t0;

    mc_log_value(LogLevel::Info, "gfx fill_ms", mc_dbg_gfx_fill_ms);
    mc_log_value(LogLevel::Info, "gfx text_ms", mc_dbg_gfx_text_ms);
}

}


// منطقه تست: آخرین سکتور ۴ کیلوبایتی فلش ۸ مگابایتی. هیچ جای دیگر فلش نوشته یا پاک نمی‌شود.
constexpr uint32_t kFlashTestAddr = 0x7FF000U;

uint8_t g_flash_buf[4096];

// تست فلش. برمی‌گرداند: ۱ = پاس
// ۲ = فلش پیدا نشد، ۳ = سکتور تست خالی نیست (چیزی نوشته یا پاک نشد)، ۴ = پاک‌کردن خطا داد،
// ۵ = بعد از پاک‌کردن همه بایت‌ها ۰xFF نشد، ۶ = نوشتن خطا داد، ۷ = بایت‌های نوشته‌شده درست نیستند،
// ۸ = بایت‌های اطراف محدوده نوشتن خراب شدند، ۹ = پاک‌کردن آخر خطا داد یا ۰xFF نشد، ۱۰ = خواندن خطا داد
uint32_t run_flash_test() {
    if (!mc_flash_init()) { return 2U; }

    // سرعت خواندن: ۶۴ کیلوبایت از ابتدای فلش (فقط خواندن)
    uint32_t t0 = mc_millis();
    for (uint32_t a = 0U; a < 65536U; a += 4096U) {
        if (!mc_flash_read(a, g_flash_buf, 4096U)) { return 10U; }
    }
    mc_dbg_flash_read_ms = mc_millis() - t0;

    // سکتور تست باید خالی باشد، وگرنه هیچ کاری نمی‌کنیم
    if (!mc_flash_read(kFlashTestAddr, g_flash_buf, 4096U)) { return 10U; }
    for (uint32_t i = 0U; i < 4096U; ++i) {
        if (g_flash_buf[i] != 0xFFU) { return 3U; }
    }

    // پاک‌کردن و بررسی
    t0 = mc_millis();
    if (!mc_flash_erase_sector(kFlashTestAddr)) { return 4U; }
    mc_dbg_flash_erase_ms = mc_millis() - t0;
    if (!mc_flash_read(kFlashTestAddr, g_flash_buf, 4096U)) { return 10U; }
    for (uint32_t i = 0U; i < 4096U; ++i) {
        if (g_flash_buf[i] != 0xFFU) { return 5U; }
    }

    // نوشتن ۳۰۰ بایت از آفست ۱۰۰ (از مرز صفحه ۲۵۶ بایتی عبور می‌کند)
    static uint8_t pattern[300];
    for (uint32_t i = 0U; i < 300U; ++i) {
        pattern[i] = static_cast<uint8_t>(((i * 7U) + 3U) & 0xFFU);
    }
    t0 = mc_millis();
    if (!mc_flash_write(kFlashTestAddr + 100U, pattern, 300U)) { return 6U; }
    mc_dbg_flash_write_ms = mc_millis() - t0;

    if (!mc_flash_read(kFlashTestAddr, g_flash_buf, 4096U)) { return 10U; }
    for (uint32_t i = 0U; i < 4096U; ++i) {
        const bool inside = (i >= 100U) && (i < 400U);
        const uint8_t expected = inside ? pattern[i - 100U] : 0xFFU;
        if (g_flash_buf[i] != expected) { return inside ? 7U : 8U; }
    }

    // پاک‌کردن آخر تا سکتور تست دوباره خالی بماند
    if (!mc_flash_erase_sector(kFlashTestAddr)) { return 9U; }
    if (!mc_flash_read(kFlashTestAddr, g_flash_buf, 4096U)) { return 10U; }
    for (uint32_t i = 0U; i < 4096U; ++i) {
        if (g_flash_buf[i] != 0xFFU) { return 9U; }
    }
    return 1U;
}

void task_flash_test() {
    if (mc_dbg_flash_test_request == 0U) { return; }
    mc_dbg_flash_test_request = 0U;
    mc_dbg_flash_test = 0U;

    const uint32_t result = run_flash_test();
    mc_dbg_flash_test = result;
    if (result == 1U) {
        mc_log(LogLevel::Info, "flash test pass");
    } else {
        mc_log_value(LogLevel::Error, "flash test failed", result);
    }
}

// اسپلش: یک بار بعد از روشن‌شدن، و هر بار که mc_dbg_splash_request برابر ۱ شود.
void task_splash() {
    static bool boot_done = false;
    if (boot_done && (mc_dbg_splash_request == 0U)) { return; }
    boot_done = true;
    mc_dbg_splash_request = 0U;
    mc_dbg_splash_result = 0U;

    const uint32_t t0 = mc_millis();
    const uint32_t result = mc_splash_show();
    mc_dbg_splash_ms = mc_millis() - t0;
    mc_dbg_splash_result = result;
    mc_ui_notify_splash(result);

    if (result == 1U) {
        mc_log(LogLevel::Info, "splash ok");
    } else {
        mc_log_value(LogLevel::Error, "splash failed", result);
    }
}

// ورودی دستورها: خط کامل را می‌گیرد، شماره خط و چک‌سامش را می‌سنجد (در خطا Resend می‌فرستد)،
// تجزیه می‌کند و دستور درست را در صف می‌گذارد. اگر صف پر باشد، خط در خط‌خوان می‌ماند
// (برداشته نمی‌شود) تا جایی در صف باز شود؛ شماره خط هم در این حالت جلو نمی‌رود.
// این وظیفه در بند ۳.۵ هم می‌ماند.
void task_cmd_intake() {
    static bool waiting = false;   // این خط قبلاً به‌خاطر پر بودن صف شمرده شده
    if (!mc_line_ready()) { return; }
    if (mc_cmdq_full()) {
        if (!waiting) {
            waiting = true;
            mc_dbg_cq_full_waits = mc_dbg_cq_full_waits + 1U;
        }
        return;
    }
    waiting = false;

    mc_log(LogLevel::Debug, mc_line_text());   // بند ۳.۸: متن هر خط دریافتی فقط در سطح Debug به USB برمی‌گردد

    static char cmd_text[kLineMaxChars + 1U];   // متن دستور بعد از حذف N و *
    const LineCheck chk = mc_line_check(mc_line_text(), cmd_text, sizeof(cmd_text));
    mc_line_release();
    if (chk != LineCheck::Ok) {
        mc_line_send_resend(chk);
        return;
    }

    static GcodeCommand cmd;   // static چون حدود ۲۴۰ بایت است
    const GcodeResult r = mc_gcode_parse(cmd_text, cmd);
    mc_dbg_gc_result = static_cast<uint32_t>(r);
    if (r == GcodeResult::Ok) {
        if ((cmd.letter == 'M') && (cmd.number == 108U) && (cmd.sub == -1) && mc_filchg_waiting_user()) {
            mc_filchg_resume_request(3U);
            mc_reply_ok();
        } else {
            if (!mc_cmdq_push(cmd)) {
                mc_log(LogLevel::Error, "command queue push failed");
                mc_reply_error("Command queue full");
            }
        }
    } else if (r == GcodeResult::Empty) {
        mc_reply_ok();   // خط فقط کامنت بود: کامپیوتر هم ok می‌خواهد
    } else {
        mc_log_value(LogLevel::Error, "gcode parse error", static_cast<uint32_t>(r));
        if (r == GcodeResult::BadParam) {
            mc_reply_error("Bad parameter");
        } else if (r == GcodeResult::TooManyParams) {
            mc_reply_error("Too many parameters");
        } else {
            mc_reply_error("Bad command word");
        }
    }
}

}  // namespace mc

extern "C" void mc_core_init_c(void) {
    mc::mc_log(mc::LogLevel::Info, "core ready");
    mc::mc_output_init();   // بند ۴.۳: PWM هیتر و فن (همه خاموش)
    mc::mc_stepper_out_init();   // بند ۵.۱: خروجی‌های استپر (همه خاموش)
    mc::mc_stepgen_init();   // بند ۵.۲
    mc::mc_axis_move_init();   // بند ۵.۳
    mc::mc_mq_init();   // بند ۵.۶
    mc::mc_mode_init();   // بند ۵.۸
    mc::mc_extruder_init();   // بند ۵.۹
    mc::mc_softlimit_init();   // بند ۵.۱۰
    mc::mc_mlim_init();   // بند ۵.۱۱
    mc::mc_gmove_init();  // بند ۵.۱۱
    mc::mc_endstop_init();   // بند ۶.۱
    mc::mc_eguard_init();   // بند ۶.۲
    mc::mc_homing_init();   // بند ۶.۳
    mc::mc_bltouch_init();   // بند ۶.۵
    mc::mc_filament_init();  // بند ۸.۱۶.۴: سنسور فیلامنت
    mc::mc_filchg_init();    // بند ۹.۱: تعویض فیلامنت
    mc::mc_fload_init();     // بند ۹.۲: بارگذاری و تخلیه فیلامنت
    mc::mc_filrun_init();    // بند ۹.۴: واکنش به اتمام فیلامنت
    mc::mc_print_init();     // بند ۹.۴: وضعیت چاپ
    mc::mc_endstop_print_init(); // بند ۱۰.۸: بررسی اندستاپ حین چاپ
    mc::mc_alert_init();     // بند ۱۰.۱۳: هشدارها و پیام‌های سیستم
    mc::mc_probe_init();   // بند ۶.۶
    mc::mc_eeprom_init();   // بند ۷.۱
    mc::mc_settings_init();   // بند ۷.۲
    mc::mc_cfg_boot_init();   // بند ۷.۴
    mc::mc_encoder_init();   // بند ۸.۸
    mc::g_enc_pressed_at_boot = mc::mc_encoder_is_pressed();   // بند ۸.۱۵: بررسی فشردن انکودر هنگام روشن شدن
    mc::mc_touchin_init();   // بند ۸.۱۵: سرویس ورودی لمس
    mc::mc_tcal_store_init();   // بند ۸.۱۵: ذخیره‌سازی کالیبراسیون لمس
    mc::mc_dispset_init();   // بند ۸.۲۱: تنظیمات نمایشگر
    mc::mc_ui_init();   // بند ۸.۲
    mc::g_main.add(mc::task_100ms, 100U);
    mc::g_main.add(mc::task_1s, 1000U);
    mc::g_main.add(mc::task_watchdog_feed, 100U);
    mc::g_main.add(mc::task_kill_test, 100U);
    mc::g_main.add(mc::task_log_flush, 20U);
    mc::g_main.add(mc::task_log_test, 100U);
    mc::g_main.add(mc::task_heartbeat, 60000U);
    mc::g_main.add(mc::task_tft_test, 100U);
    mc::g_main.add(mc::task_gfx_test, 100U);
    mc::g_main.add(mc::mc_touchin_poll, MC_TOUCH_POLL_MS);   // بند ۸.۱۵: ورودی لمس
    mc::g_main.add(mc::mc_tcal_store_poll, MC_TOUCH_CAL_STORE_POLL_MS);   // بند ۸.۱۵: ذخیره لمس
    mc::g_main.add(mc::mc_dispset_poll, MC_DISPSET_POLL_MS);   // بند ۸.۲۱: ذخیره تنظیمات نمایشگر
    mc::g_main.add(mc::task_flash_test, 100U);
    mc::g_main.add(mc::task_splash, 100U);

    mc::g_main.add(mc::mc_encoder_poll, MC_ENC_POLL_MS);   // بند ۸.۸: انکودر و بوق
    mc::g_main.add(mc::mc_adc_sampler_poll, mc::kAdcPeriodMs);   // بند ۴.۱: نمونه‌گیری ADC هر ۱۰ میلی‌ثانیه
    mc::g_main.add(mc::mc_temp_poll, mc::kTempPeriodMs);   // بند ۴.۲: تبدیل ADC به دما هر ۱۰۰ میلی‌ثانیه
    mc::g_main.add(mc::mc_temp_limits_poll, 100U);   // بند ۴.۶: حفاظت حداقل و حداکثر دما
    mc::g_main.add(mc::mc_temp_protect_poll, 100U);   // بند ۴.۷: watch heating و thermal runaway و heater failed
    mc::g_main.add(mc::mc_output_poll, 100U);   // بند ۴.۳: به‌روزرسانی خروجی‌ها و متغیرهای debug
    mc::g_main.add(mc::mc_heater_ex_poll, 100U);   // بند ۴.۴: PID هیتر اکسترودر
    mc::g_main.add(mc::mc_heater_bed_poll, 100U);   // بند ۴.۵: کنترل هیتر بستر
    mc::g_main.add(mc::mc_temp_autoreport_poll, 100U);   // بند ۴.۸: گزارش خودکار دما (M155)
    mc::g_main.add(mc::mc_stepper_out_poll, 10U);   // بند ۵.۱: خروجی‌های استپر
    mc::g_main.add(mc::mc_stepgen_poll, 10U);   // بند ۵.۲: تولید پالس با تایمر
    mc::g_main.add(mc::mc_axis_move_poll, 10U);   // بند ۵.۳: حرکت یک محور با سرعت ثابت
    mc::g_main.add(mc::mc_mq_poll, 10U);   // بند ۵.۶: صف بلوک‌های حرکت
    mc::g_main.add(mc::mc_plan_poll, 10U);   // بند ۵.۷: برنامه‌ریز (lookahead و متغیرهای debug)
    mc::g_main.add(mc::mc_mode_poll, 100U);   // بند ۵.۸: حالت‌ها و متغیرهای debug
    mc::g_main.add(mc::mc_extruder_poll, 100U);   // بند ۵.۹: اکسترودر و متغیرهای debug
    mc::g_main.add(mc::mc_softlimit_poll, 100U);   // بند ۵.۱۰: حدود نرم‌افزاری و متغیرهای debug
    mc::g_main.add(mc::mc_mlim_poll, 100U);   // بند ۵.۱۱
    mc::g_main.add(mc::mc_gmove_poll, 100U);  // بند ۵.۱۱
    mc::g_main.add(mc::mc_endstop_poll, mc::kEndstopPollMs);   // بند ۶.۱: خواندن اندستاپ‌ها هر ۱ میلی‌ثانیه
    mc::g_main.add(mc::mc_eguard_poll, mc::kEguardPollMs);   // بند ۶.۲: پردازش توقف اندستاپ
    mc::g_main.add(mc::mc_homing_poll, 100U);   // بند ۶.۳: متغیرهای debug هومینگ
    mc::g_main.add(mc::mc_bltouch_poll, mc::kBltouchPeriodMs);   // بند ۶.۵: BLTouch و متغیرهای debug
    mc::g_main.add(mc::mc_filament_poll, 10U);   // بند ۸.۱۶.۴: سنسور فیلامنت
    mc::g_main.add(mc::mc_filchg_poll, 100U);   // بند ۹.۱: تعویض فیلامنت
    mc::g_main.add(mc::mc_fload_poll, 100U);    // بند ۹.۲: بارگذاری و تخلیه فیلامنت
    mc::g_main.add(mc::mc_filrun_poll, 100U);   // بند ۹.۴: واکنش به اتمام فیلامنت
    mc::g_main.add(mc::mc_print_poll, 100U);    // بند ۹.۴: وضعیت چاپ
    mc::g_main.add(mc::mc_endstop_print_poll, mc::kEndstopPrintPollMs); // بند ۱۰.۸: بررسی اندستاپ حین چاپ
    mc::g_main.add(mc::mc_alert_poll, 100U);    // بند ۱۰.۱۳: پایش متغیر دیباگ alert
    mc::g_main.add(mc::mc_probe_poll, 100U);   // بند ۶.۶: G30 و متغیرهای debug
    mc::g_main.add(mc::mc_eeprom_poll, mc::kEepromPollMs);   // بند ۷.۱: درایور EEPROM
    mc::g_main.add(mc::mc_settings_poll, mc::kSettingsPollMs);   // بند ۷.۲: ذخیره‌سازی سخت‌دار
    mc::g_main.add(mc::mc_cfg_boot_poll, mc::kCfgBootPollMs);   // بند ۷.۴: خواندن خودکار تنظیمات هنگام بوت
    mc::g_main.add(mc::mc_ui_poll, mc::kUiPollMs);   // بند ۸.۲: صفحه اصلی
    mc::g_main.add(mc::mc_line_poll, 0U);
    mc::g_main.add(mc::task_cmd_intake, 0U);
    mc::g_main.add(mc::mc_dispatch_poll, 0U);
}

extern "C" void mc_core_loop_c(void) {
    mc::g_main.run();
}
