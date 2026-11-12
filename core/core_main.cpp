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

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_sched_100ms       = 0;  // هر ۱۰۰ میلی‌ثانیه یکی زیاد می‌شود
volatile uint32_t mc_dbg_sched_1s          = 0;  // هر ۱ ثانیه یکی زیاد می‌شود
volatile uint32_t mc_dbg_sched_max_task_us = 0;  // طولانی‌ترین اجرای یک وظیفه (میکروثانیه)
volatile uint32_t mc_dbg_kill_request      = 0;  // برای تست: نوشتن عدد ۱ در این متغیر، kill را اجرا می‌کند
volatile uint32_t mc_dbg_log_test_request  = 0;  // برای تست: نوشتن عدد ۱ در این متغیر، چند خط آزمایشی لاگ می‌کند
volatile uint32_t mc_dbg_uptime_s          = 0;  // ثانیه از روشن‌شدن برد
volatile uint32_t mc_dbg_heartbeat_count   = 0;  // تعداد دفعاتی که لاگ ۶۰ ثانیه‌ای اجرا شده
volatile uint32_t mc_dbg_tft_test_request  = 0;  // برای تست: نوشتن عدد ۱ در این متغیر، تست نمایشگر را اجرا می‌کند
volatile uint32_t mc_dbg_tft_step          = 0;  // مرحله جاری تست نمایشگر (۰ = بیکار)
volatile uint32_t mc_dbg_gfx_test_request  = 0;  // برای تست: نوشتن عدد ۱ در این متغیر، تست گرافیک را اجرا می‌کند
volatile uint32_t mc_dbg_gfx_fill_ms       = 0;  // زمان پر کردن کل صفحه (میلی‌ثانیه)
volatile uint32_t mc_dbg_gfx_text_ms       = 0;  // زمان نوشتن تمام تست‌های متن (میلی‌ثانیه)
volatile uint32_t mc_dbg_touch_cal_request = 0;  // برای تست: نوشتن عدد ۱ در این متغیر، کالیبراسیون لمس را شروع می‌کند
volatile uint32_t mc_dbg_touch_state       = 0;  // مرحله تست (۰ بیکار، ۱ تا ۳ انتظار برای نقطه‌ها، ۴ انتظار رها شدن، ۵ نقاشی)
volatile uint32_t mc_dbg_touch_pressed     = 0;  // ۱ = الان صفحه لمس شده
volatile uint32_t mc_dbg_touch_raw_x       = 0;  // آخرین مختصات خام X
volatile uint32_t mc_dbg_touch_raw_y       = 0;  // آخرین مختصات خام Y
volatile uint32_t mc_dbg_touch_x           = 0;  // آخرین مختصات پیکسل X (بعد از کالیبراسیون)
volatile uint32_t mc_dbg_touch_y           = 0;  // آخرین مختصات پیکسل Y (بعد از کالیبراسیون)
volatile int      mc_dbg_touch_cal_ok      = -1; // -1 = هنوز انجام نشده، 1 = موفق، 0 = ناموفق
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
    mc_dbg_sched_1s = mc_dbg_sched_1s + 1U;
    mc_dbg_sched_max_task_us = g_main.max_task_us();
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

// سه نقطه هدف کالیبراسیون روی صفحه (پیکسل)
constexpr uint16_t kCalX[3] = { 40U, 440U, 240U };
constexpr uint16_t kCalY[3] = { 40U, 160U, 280U };

// یک علامت + به اندازه ۲۱ در ۲۱ پیکسل
void draw_cross(uint16_t cx, uint16_t cy, uint16_t color) {
    mc_gfx_hline(static_cast<uint16_t>(cx - 10U), cy, 21U, color);
    mc_gfx_vline(cx, static_cast<uint16_t>(cy - 10U), 21U, color);
}

// صفحه هدف شماره i (۰ تا ۲) را نشان می‌دهد
void show_target(uint8_t i) {
    static const char* const kTitle[3] = { "Point 1 of 3", "Point 2 of 3", "Point 3 of 3" };
    mc_display_fill_screen(kColorBlack);
    mc_gfx_text(12U, 12U, "Touch the red cross", kColorWhite, kColorBlack, 2U);
    mc_gfx_text(12U, 40U, kTitle[i], kColorWhite, kColorBlack, 2U);
    draw_cross(kCalX[i], kCalY[i], kColorRed);
}

void task_touch_test() {
    // مرحله‌ها: ۰ بیکار، ۱ تا ۳ انتظار فشردن نقطه ۱ تا ۳، ۴ انتظار رها شدن انگشت، ۵ نقاشی
    static uint8_t  state         = 0U;
    static uint8_t  point         = 0U;
    static bool     inited        = false;
    static uint16_t raw_x[3]      = { 0U, 0U, 0U };
    static uint16_t raw_y[3]      = { 0U, 0U, 0U };
    static uint8_t  release_count = 0U;
    static uint32_t paint_start   = 0U;

    // خواندن لمس (فقط بعد از راه‌اندازی)
    uint16_t rx = 0U;
    uint16_t ry = 0U;
    bool pressed = false;
    if (inited) {
        pressed = mc_touch_read_raw(&rx, &ry);
        mc_dbg_touch_pressed = pressed ? 1U : 0U;
        if (pressed) {
            mc_dbg_touch_raw_x = rx;
            mc_dbg_touch_raw_y = ry;
        }
    }

    if (state == 0U) {
        if (mc_dbg_touch_cal_request == 0U) { return; }
        mc_dbg_touch_cal_request = 0U;
        (void)mc_touch_init();
        inited = true;
        if (!mc_display_init()) {
            mc_log(LogLevel::Error, "display init failed");
            return;
        }
        mc_display_fill_screen(kColorBlack);
        mc_display_backlight(true);
        mc_dbg_touch_cal_ok = -1;
        point = 0U;
        show_target(point);
        state = 1U;
        mc_dbg_touch_state = state;
        return;
    }

    if ((state >= 1U) && (state <= 3U)) {
        if (!pressed) { return; }
        raw_x[point] = rx;
        raw_y[point] = ry;
        draw_cross(kCalX[point], kCalY[point], kColorGreen);   // نقطه گرفته شد
        release_count = 0U;
        state = 4U;
        mc_dbg_touch_state = state;
        return;
    }

    if (state == 4U) {
        if (pressed) {
            release_count = 0U;
            return;
        }
        release_count = static_cast<uint8_t>(release_count + 1U);
        if (release_count < 5U) { return; }   // پنج بار پشت‌سرهم رها (حدود ۱۰۰ میلی‌ثانیه)

        point = static_cast<uint8_t>(point + 1U);
        if (point < 3U) {
            show_target(point);
            state = static_cast<uint8_t>(point + 1U);
            mc_dbg_touch_state = state;
            return;
        }

        // هر سه نقطه گرفته شد؛ ضرایب را حساب کن
        const bool ok = mc_touch_calibrate_3pt(raw_x, raw_y, kCalX, kCalY);
        mc_dbg_touch_cal_ok = ok ? 1 : 0;
        if (!ok) {
            mc_log(LogLevel::Error, "touch calibration failed");
            mc_gfx_text(12U, 100U, "Calibration failed", kColorRed, kColorBlack, 2U);
            state = 0U;
            mc_dbg_touch_state = state;
            return;
        }

        // صفحه نقاشی: پنج علامت + برای سنجیدن دقت و یک پیام
        mc_display_fill_screen(kColorBlack);
        draw_cross(20U, 20U, kColorWhite);
        draw_cross(460U, 20U, kColorWhite);
        draw_cross(20U, 300U, kColorWhite);
        draw_cross(460U, 300U, kColorWhite);
        draw_cross(240U, 160U, kColorWhite);
        mc_gfx_text(110U, 80U, "Paint test (30 s)", kColorGreen, kColorBlack, 2U);
        paint_start = mc_millis();
        state = 5U;
        mc_dbg_touch_state = state;
        return;
    }

    if (state == 5U) {
        if (mc_time_elapsed_ms(paint_start, 30000U)) {
            mc_gfx_text(110U, 110U, "Done", kColorGreen, kColorBlack, 2U);
            state = 0U;
            mc_dbg_touch_state = state;
            return;
        }
        if (!pressed) { return; }
        uint16_t sx = 0U;
        uint16_t sy = 0U;
        if (mc_touch_apply(rx, ry, &sx, &sy)) {
            mc_dbg_touch_x = sx;
            mc_dbg_touch_y = sy;
            const uint16_t dx = (sx >= 2U) ? static_cast<uint16_t>(sx - 2U) : 0U;
            const uint16_t dy = (sy >= 2U) ? static_cast<uint16_t>(sy - 2U) : 0U;
            mc_display_fill_rect(dx, dy, 5U, 5U, kColorWhite);   // نقطه ۵ در ۵ سفید
        }
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
        if (!mc_cmdq_push(cmd)) {
            mc_log(LogLevel::Error, "command queue push failed");
            mc_reply_error("Command queue full");
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



}  // namespace
}  // namespace mc

extern "C" void mc_core_init_c(void) {
    mc::mc_log(mc::LogLevel::Info, "core ready");
    mc::mc_output_init();   // بند ۴.۳: PWM هیتر و فن (همه خاموش)
    mc::g_main.add(mc::task_100ms, 100U);
    mc::g_main.add(mc::task_1s, 1000U);
    mc::g_main.add(mc::task_watchdog_feed, 100U);
    mc::g_main.add(mc::task_kill_test, 100U);
    mc::g_main.add(mc::task_log_flush, 20U);
    mc::g_main.add(mc::task_log_test, 100U);
    mc::g_main.add(mc::task_heartbeat, 60000U);
    mc::g_main.add(mc::task_tft_test, 100U);
    mc::g_main.add(mc::task_gfx_test, 100U);
    mc::g_main.add(mc::task_touch_test, 20U);
    mc::g_main.add(mc::task_flash_test, 100U);
    mc::g_main.add(mc::task_splash, 100U);
    mc::g_main.add(mc::mc_adc_sampler_poll, mc::kAdcPeriodMs);   // بند ۴.۱: نمونه‌گیری ADC هر ۱۰ میلی‌ثانیه
    mc::g_main.add(mc::mc_temp_poll, mc::kTempPeriodMs);   // بند ۴.۲: تبدیل ADC به دما هر ۱۰۰ میلی‌ثانیه
    mc::g_main.add(mc::mc_output_poll, 100U);   // بند ۴.۳: به‌روزرسانی خروجی‌ها و متغیرهای debug
    mc::g_main.add(mc::mc_heater_ex_poll, 100U);   // بند ۴.۴: PID هیتر اکسترودر
    mc::g_main.add(mc::mc_line_poll, 0U);
    mc::g_main.add(mc::task_cmd_intake, 0U);
    mc::g_main.add(mc::mc_dispatch_poll, 0U);
}

extern "C" void mc_core_loop_c(void) {
    mc::g_main.run();
}
