// صفحه سنسورها رابط کاربری (بند ۸.۱۲).
// هیچ هدر ST اینجا include نمی‌شود. فقط از hal و gfx و encoder و endstop و bltouch و hal_gpio استفاده می‌کند.
#include "ui_sensors.h"
#include "ui_home.h"
#include "ui_colors.h"
#include "ui_touch.h"
#include "../core/gfx.h"
#include "../core/text_format.h"
#include "../core/endstop.h"
#include "../core/bltouch.h"
#include "../core/filament_sensor.h"
#include "../core/error.h"
#include "../core/command_queue.h"
#include "../core/gcode_parser.h"
#include "../core/machine_state.h"
#include "../gcode/gcode_dispatch.h"
#include "../hal/hal_gpio.h"
#include "../core/encoder.h"
#include "../hal/hal_display.h"
#include "../hal/hal_buzzer.h"
#include "../hal/hal_time.h"
#include "../config/mc_config.h"

extern "C" {
extern volatile uint32_t mc_dbg_297_ui_fake_on;

volatile uint32_t mc_dbg_344_sen_sel_shown     = 4;   // خواندنی: اندیس مورد فوکوس‌شده (۰ تا ۴)
volatile uint32_t mc_dbg_345_sen_enter_cnt     = 0;   // خواندنی: تعداد اینترها روی مورد غیر BACK
volatile uint32_t mc_dbg_346_sen_enter_idx     = 255; // خواندنی: اندیس مورد آخرین اینتر (۲۵۵ = هنوز نشده)
volatile uint32_t mc_dbg_347_sen_force         = 0;   // نوشتنی: ۰ = بی‌اثر؛ ۱ = باز کردن صفحه سنسورها؛ ۲ = برگشت به منو؛ ۳ تا ۷ = فوکوس روی مورد (مقدار - ۳)
volatile uint32_t mc_dbg_348_sen_fake          = 0;   // نوشتنی: حالت ساختگی ظاهر صفحه سنسورها
volatile uint32_t mc_dbg_401_sen_act_cnt       = 0;   // خواندنی: تعداد دفعاتی که دکمه‌ها واقعاً کاری انجام دادند
volatile uint32_t mc_dbg_402_sen_act_last      = 255; // خواندنی: اندیس آخرین دکمه موفق (0 = DEPLOY، 1 = STOW، 2 = TEST، 3 = ALM CLR؛ 255 = هنوز نشده)
volatile uint32_t mc_dbg_403_sen_act_refused   = 0;   // خواندنی: تعداد دفعاتی که دکمه رد شد
volatile uint32_t mc_dbg_404_sen_act_en        = 1;   // نوشتنی: 0 = دکمه‌ها هیچ کاری نمی‌کنند (پیش‌فرض 1)
volatile uint32_t mc_dbg_405_sen_refuse_reason = 0;   // خواندنی: علت آخرین رد شدن
}

namespace mc {
namespace {

constexpr uint16_t kSelBg  = mc_rgb565(24, 38, 64);
constexpr uint16_t kAxisX  = mc_rgb565(240, 90, 90);
constexpr uint16_t kAxisY  = mc_rgb565(90, 200, 110);
constexpr uint16_t kAxisZ  = mc_rgb565(90, 150, 255);
constexpr uint16_t kWarn   = mc_rgb565(255, 170, 0);

static uint8_t g_sen_sel        = 4U; // فوکوس اولیه روی BACK (اندیس ۴)
static uint8_t g_sen_step       = 0U; // گام نقاشی ورود (۰ تا ۵، ۲۵۵ = کار عادی)
static uint8_t g_sen_live_cycle = 0U; // چرخه‌گیری به‌روزرسانی ۵ بخش در حالت عادی (۰ تا ۴)

static bool    g_cached_endstop_hit[3] = { false, false, false };
static bool    g_cached_bltouch_busy   = false;
static uint8_t g_cached_bltouch_pin    = 255U;
static bool    g_cached_fil_empty      = false;

static bool     g_msg_shown    = false;   // پیام علت رد شدن به‌جای عنوان نوار بالا
static uint32_t g_msg_start_ms = 0U;      // زمان شروع نمایش پیام

static void draw_dot(uint16_t cx, uint16_t cy, uint16_t color) {
    mc_gfx_fill_rect(static_cast<uint16_t>(cx - 2U), static_cast<uint16_t>(cy - 4U), 5U, 9U, color);
    mc_gfx_fill_rect(static_cast<uint16_t>(cx - 3U), static_cast<uint16_t>(cy - 3U), 7U, 7U, color);
    mc_gfx_fill_rect(static_cast<uint16_t>(cx - 4U), static_cast<uint16_t>(cy - 2U), 9U, 5U, color);
}

// توابع خواندن داده‌ها (با در نظر گرفتن حالت ساختگی)
static bool read_endstop_hit(uint8_t axis) {
    const uint32_t fake = mc_dbg_348_sen_fake;
    if ((fake & 0x80U) != 0U) { // بیت ۷ = فعال بودن fake
        return (fake & (1U << axis)) != 0U;
    }
    // اندستاپ‌ها NC هستند: true یعنی فشرده یا قطع‌سیم
    return mc_endstop_triggered(axis);
}

static bool read_bltouch_busy() {
    const uint32_t fake = mc_dbg_348_sen_fake;
    if ((fake & 0x80U) != 0U) {
        return (fake & 0x40U) != 0U; // بیت ۶
    }
    return mc_bltouch_busy();
}

static uint8_t read_bltouch_pin_code() {
    const uint32_t fake = mc_dbg_348_sen_fake;
    if ((fake & 0x80U) != 0U) {
        return static_cast<uint8_t>((fake >> 4U) & 0x03U); // بیت ۴ و ۵
    }
    const BltouchPin st = mc_bltouch_pin_state();
    return static_cast<uint8_t>(st);
}

static bool read_filament_empty() {
    const uint32_t fake = mc_dbg_348_sen_fake;
    if ((fake & 0x80U) != 0U) {
        return (fake & 0x08U) != 0U; // بیت ۳
    }
    return mc_filament_empty();
}

// ۱. کارت‌های اندستاپ (غیرقابل انتخاب)
// اندستاپ‌ها NC هستند. TRIG یعنی فشرده یا قطع‌سیم، FREE یعنی آزاد (بند ۸.۱۲)
static void draw_endstop_card(uint8_t axis) {
    if (axis >= 3U) { return; }

    static const uint16_t kCardX[3] = { 8U, 166U, 324U };
    static const char*    kAxisLabel[3] = { "X", "Y", "Z" };
    static const uint16_t kAxisColor[3] = { kAxisX, kAxisY, kAxisZ };

    const uint16_t cx = kCardX[axis];
    constexpr uint16_t cy = 46U;
    constexpr uint16_t cw = 148U;
    constexpr uint16_t ch = 44U;

    const bool hit = read_endstop_hit(axis);
    const char* st_text = hit ? "TRIG" : "FREE";
    const uint16_t st_color = hit ? kColorRed : kColorGreen;

    mc_gfx_fill_rect(cx, cy, cw, ch, kCardBg);
    mc_gfx_rect(cx, cy, cw, ch, kCardBorder);

    // حرف محور
    mc_gfx_text(static_cast<uint16_t>(cx + 12U), static_cast<uint16_t>(cy + 12U),
                kAxisLabel[axis], kAxisColor[axis], kCardBg, 3U);

    // متن وضعیت راست‌چین
    const uint16_t tw = mc_gfx_text_width(st_text, 2U);
    const uint16_t tx = static_cast<uint16_t>(cx + cw - 10U - tw);
    const uint16_t ty = static_cast<uint16_t>(cy + 15U);
    mc_gfx_text(tx, ty, st_text, st_color, kCardBg, 2U);

    g_cached_endstop_hit[axis] = hit;
}

// ۲. برچسب و نشان وضعیت BLTouch
static void draw_bltouch_badge() {
    mc_gfx_text(20U, 104U, "BLTOUCH", kLabelGray, kScreenBg, 2U);

    const bool busy = read_bltouch_busy();
    const uint8_t pin_code = read_bltouch_pin_code();

    const char* st_text = "UNKNOWN";
    uint16_t bg_col = kCardBg;
    uint16_t fg_col = kLabelGray;

    if (busy) {
        st_text = "BUSY";
        bg_col  = kYellowBusy;
        fg_col  = kScreenBg;
    } else if (pin_code == 1U) { // Deployed
        st_text = "DEPLOYED";
        bg_col  = kYellowBusy;
        fg_col  = kScreenBg;
    } else if (pin_code == 2U) { // Stowed
        st_text = "STOWED";
        bg_col  = kColorGreen;
        fg_col  = kScreenBg;
    } else { // Unknown
        st_text = "UNKNOWN";
        bg_col  = kCardBg;
        fg_col  = kLabelGray;
    }

    constexpr uint16_t bx = 352U;
    constexpr uint16_t by = 100U;
    constexpr uint16_t bw = 112U;
    constexpr uint16_t bh = 22U;

    mc_gfx_fill_rect(bx, by, bw, bh, bg_col);

    const uint16_t tw = mc_gfx_text_width(st_text, 2U);
    const uint16_t tx = static_cast<uint16_t>(bx + (bw - tw) / 2U);
    constexpr uint16_t ty = 103U;
    mc_gfx_text(tx, ty, st_text, fg_col, bg_col, 2U);

    g_cached_bltouch_busy = busy;
    g_cached_bltouch_pin  = pin_code;
}

// ۳. ردیف فیلامنت (غیرقابل انتخاب)
static void draw_filament_row() {
    constexpr uint16_t bx = 8U;
    constexpr uint16_t by = 182U;
    constexpr uint16_t bw = 464U;
    constexpr uint16_t bh = 40U;

    mc_gfx_fill_rect(bx, by, bw, bh, kCardBg);
    mc_gfx_rect(bx, by, bw, bh, kCardBorder);

    mc_gfx_text(22U, 195U, "FILAMENT", kLabelGray, kCardBg, 2U);

    const bool empty = read_filament_empty();
    const char* st_text = empty ? "EMPTY" : "OK";
    const uint16_t st_color = empty ? kColorRed : kColorGreen;

    const uint16_t tw = mc_gfx_text_width(st_text, 2U);
    const uint16_t tx = static_cast<uint16_t>(462U - tw);
    mc_gfx_text(tx, 195U, st_text, st_color, kCardBg, 2U);

    const uint16_t dot_cx = static_cast<uint16_t>(tx - 16U);
    draw_dot(dot_cx, 202U, st_color);

    g_cached_fil_empty = empty;
}

// ۴. دکمه‌های قابل فوکوس
static void draw_sen_item(uint8_t item_idx) {
    if (item_idx >= 5U) { return; }
    const bool focused = (item_idx == g_sen_sel);

    if (item_idx <= 3U) {
        static const uint16_t kBtnX[4] = { 8U, 126U, 244U, 362U };
        static const char*    kBtnLabel[4] = { "DEPLOY", "STOW", "TEST", "ALM CLR" };

        const uint16_t bx = kBtnX[item_idx];
        constexpr uint16_t by = 128U;
        constexpr uint16_t bw = 110U;
        constexpr uint16_t bh = 40U;

        uint16_t bg_col     = focused ? kSelBg : kCardBg;
        uint16_t fg_col     = focused ? kColorWhite : kLabelGray;
        uint16_t border_col = focused ? kAccentLine : kCardBorder;

        // سبک ویژه برای دکمه ALM CLR (اندیس ۳)
        if (item_idx == 3U) {
            bg_col     = focused ? mc_rgb565(60, 40, 8) : kCardBg;
            fg_col     = focused ? kColorWhite : kWarn;
            border_col = kWarn;
        }

        mc_gfx_fill_rect(bx, by, bw, bh, bg_col);
        mc_gfx_rect(bx, by, bw, bh, border_col);
        if (focused) {
            mc_gfx_rect(static_cast<uint16_t>(bx + 1U), static_cast<uint16_t>(by + 1U),
                        static_cast<uint16_t>(bw - 2U), static_cast<uint16_t>(bh - 2U), border_col);
        }

        const uint16_t tw = mc_gfx_text_width(kBtnLabel[item_idx], 2U);
        const uint16_t tx = static_cast<uint16_t>(bx + (bw - tw) / 2U);
        const uint16_t ty = static_cast<uint16_t>(by + (bh - 16U) / 2U);
        mc_gfx_text(tx, ty, kBtnLabel[item_idx], fg_col, bg_col, 2U);
    } else if (item_idx == 4U) { // BACK Button
        constexpr uint16_t bx = 324U;
        constexpr uint16_t by = 258U;
        constexpr uint16_t bw = 148U;
        constexpr uint16_t bh = 44U;

        const uint16_t bg_col     = focused ? kSelBg : kCardBg;
        const uint16_t fg_col     = focused ? kColorWhite : kLabelGray;
        const uint16_t border_col = focused ? kAccentLine : kCardBorder;

        mc_gfx_fill_rect(bx, by, bw, bh, bg_col);
        mc_gfx_rect(bx, by, bw, bh, border_col);
        if (focused) {
            mc_gfx_rect(static_cast<uint16_t>(bx + 1U), static_cast<uint16_t>(by + 1U),
                        static_cast<uint16_t>(bw - 2U), static_cast<uint16_t>(bh - 2U), border_col);
        }

        const uint16_t tw = mc_gfx_text_width("BACK", 2U);
        const uint16_t tx = static_cast<uint16_t>(bx + (bw - tw) / 2U);
        const uint16_t ty = static_cast<uint16_t>(by + (bh - 16U) / 2U);
        mc_gfx_text(tx, ty, "BACK", fg_col, bg_col, 2U);
    }
}

static void apply_sen_selection(uint8_t target_sel) {
    if (target_sel >= 5U) { return; }
    const uint8_t old_sel = g_sen_sel;
    g_sen_sel = target_sel;
    mc_dbg_344_sen_sel_shown = g_sen_sel;

    if (old_sel != g_sen_sel) {
        draw_sen_item(old_sel);
        draw_sen_item(g_sen_sel);
    }
}

// بند ۸.۱۶: پیام علت رد شدن به‌جای عنوان نوار بالا و بوق بلند
static void sen_refuse_feedback(uint32_t reason) {
    const char* txt = "REFUSED";
    switch (reason) {
        case 2U: txt = "KILLED";             break;
        case 3U: txt = "FAKE MODE ON";       break;
        case 4U: txt = "BUTTONS DISABLED";   break;
        case 7U: txt = "WAIT: JOB RUNNING";  break;
        case 8U: txt = "QUEUE ERROR";        break;
        default: break;
    }
    mc_gfx_fill_rect(0U, 0U, 480U, 36U, kTopBarBg);
    const uint16_t tw = mc_gfx_text_width(txt, 2U);
    const uint16_t tx = static_cast<uint16_t>((480U - tw) / 2U);
    mc_gfx_text(tx, 10U, txt, kWarn, kTopBarBg, 2U);
    g_msg_shown    = true;
    g_msg_start_ms = mc_millis();
    mc_buzzer_click(MC_UI_REFUSE_BEEP_MS);   // غیرمسدودکننده؛ بوق کوتاه لمس یا انکودر را با بوق بلند عوض می‌کند
}

// بعد از MC_UI_SEN_MSG_MS میلی‌ثانیه عنوان SENSORS را برمی‌گرداند
static void sen_msg_poll() {
    if (g_msg_shown && ((mc_millis() - g_msg_start_ms) >= static_cast<uint32_t>(MC_UI_SEN_MSG_MS))) {
        mc_gfx_fill_rect(0U, 0U, 480U, 36U, kTopBarBg);
        const uint16_t tw = mc_gfx_text_width("SENSORS", 2U);
        const uint16_t tx = static_cast<uint16_t>((480U - tw) / 2U);
        mc_gfx_text(tx, 10U, "SENSORS", kColorWhite, kTopBarBg, 2U);
        g_msg_shown = false;
    }
}

static void sen_note_refuse(uint32_t reason) {
    mc_dbg_403_sen_act_refused   = mc_dbg_403_sen_act_refused + 1U;
    mc_dbg_405_sen_refuse_reason = reason;
    sen_refuse_feedback(reason);
}

static void sen_note_ok(uint8_t item_idx) {
    mc_dbg_401_sen_act_cnt       = mc_dbg_401_sen_act_cnt + 1U;
    mc_dbg_402_sen_act_last      = item_idx;
    mc_dbg_405_sen_refuse_reason = 0U;
}

static bool sen_push_line(const char* line) {
    static GcodeCommand cmd;
    if (mc_gcode_parse(line, cmd) != GcodeResult::Ok) { return false; }
    return mc_cmdq_push(cmd);
}

static bool sen_job_busy() {
    return (!mc_cmdq_empty()) || mc_dispatch_busy() || mc_motion_busy() || mc_bltouch_busy();
}

static void sen_run_action(uint8_t item_idx) {
    if (item_idx > 3U) { return; }

    // شرط‌های رد شدن به ترتیب
    if (mc_is_killed()) {
        sen_note_refuse(2U);
        return;
    }
    if ((mc_dbg_297_ui_fake_on != 0U) || ((mc_dbg_348_sen_fake & 0x80U) != 0U)) {
        sen_note_refuse(3U);
        return;
    }
    if (mc_dbg_404_sen_act_en == 0U) {
        sen_note_refuse(4U);
        return;
    }
    if (sen_job_busy()) {
        sen_note_refuse(7U);
        return;
    }

    const char* line = nullptr;
    if (item_idx == 0U) {
        line = "M401";
    } else if (item_idx == 1U) {
        line = "M402";
    } else if (item_idx == 2U) {
        line = "M280 P0 S120";
    } else if (item_idx == 3U) {
        line = "M280 P0 S160";
    }

    if ((line == nullptr) || (!sen_push_line(line))) {
        sen_note_refuse(8U);
        return;
    }

    sen_note_ok(item_idx);
}

}  // namespace

void mc_ui_sensors_open() {
    g_sen_sel        = 4U; // فوکوس اولیه روی BACK (اندیس ۴)
    g_sen_step       = 0U;
    g_sen_live_cycle = 0U;
    g_msg_shown      = false;
    g_msg_start_ms   = 0U;

    g_cached_endstop_hit[0] = false;
    g_cached_endstop_hit[1] = false;
    g_cached_endstop_hit[2] = false;
    g_cached_bltouch_busy   = false;
    g_cached_bltouch_pin    = 255U;
    g_cached_fil_empty      = false;

    mc_dbg_344_sen_sel_shown = 4U;
    mc_dbg_345_sen_enter_cnt = 0U;
    mc_dbg_346_sen_enter_idx = 255U;
    mc_dbg_347_sen_force     = 0U;
    mc_dbg_348_sen_fake      = 0U;

    (void)mc_encoder_take_clicks();
    (void)mc_encoder_take_press();
    mc_ui_touch_reset();
}

SensorsResult mc_ui_sensors_poll() {
    (void)mc_dbg_344_sen_sel_shown;
    (void)mc_dbg_345_sen_enter_cnt;
    (void)mc_dbg_346_sen_enter_idx;
    (void)mc_dbg_347_sen_force;
    (void)mc_dbg_348_sen_fake;
    (void)mc_dbg_401_sen_act_cnt;
    (void)mc_dbg_402_sen_act_last;
    (void)mc_dbg_403_sen_act_refused;
    (void)mc_dbg_404_sen_act_en;
    (void)mc_dbg_405_sen_refuse_reason;

    if (mc_dbg_347_sen_force != 0U) {
        const uint32_t force_val = mc_dbg_347_sen_force;
        mc_dbg_347_sen_force = 0U;

        if (force_val == 2U) {
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return SensorsResult::Back;
        } else if ((force_val >= 3U) && (force_val <= 7U)) {
            const uint8_t target_sel = static_cast<uint8_t>(force_val - 3U);
            if (target_sel != g_sen_sel) {
                apply_sen_selection(target_sel);
            }
        }
    }

    if (g_sen_step < 255U) {
        switch (g_sen_step) {
            case 0:
                mc_display_fill_screen(kScreenBg);
                g_sen_step = 1U;
                return SensorsResult::Stay;

            case 1: {
                mc_gfx_fill_rect(0U, 0U, 480U, 36U, kTopBarBg);
                mc_gfx_fill_rect(0U, 36U, 480U, 2U, kAccentLine);
                const uint16_t tw = mc_gfx_text_width("SENSORS", 2U);
                const uint16_t tx = static_cast<uint16_t>((480U - tw) / 2U);
                mc_gfx_text(tx, 10U, "SENSORS", kColorWhite, kTopBarBg, 2U);
                g_sen_step = 2U;
                return SensorsResult::Stay;
            }

            case 2:
                draw_endstop_card(0U);
                draw_endstop_card(1U);
                draw_endstop_card(2U);
                g_sen_step = 3U;
                return SensorsResult::Stay;

            case 3:
                draw_bltouch_badge();
                draw_sen_item(0U);
                draw_sen_item(1U);
                draw_sen_item(2U);
                draw_sen_item(3U);
                g_sen_step = 4U;
                return SensorsResult::Stay;

            case 4:
                draw_filament_row();
                g_sen_step = 5U;
                return SensorsResult::Stay;

            case 5:
                draw_sen_item(4U);
                g_sen_step = 255U;
                return SensorsResult::Stay;

            default:
                g_sen_step = 255U;
                return SensorsResult::Stay;
        }
    }

    bool touch_enter = false;
    // لمس (بند ۸.۱۶): ضربه روی دکمه‌های صفحه سنسورها
    {
        const TouchGesture tg = mc_ui_touch_poll();
        if (tg.evt == TouchEvt::Tap) {
            if (mc_ui_touch_in_rect(tg.x, tg.y, 324U, 258U, 148U, 44U)) {
                mc_ui_touch_accept(20U);
                (void)mc_encoder_take_clicks();
                (void)mc_encoder_take_press();
                return SensorsResult::Back;
            }

            int32_t hit = -1;   // اندیس دکمه عملیاتی (0 تا 3)
            if (mc_ui_touch_in_rect(tg.x, tg.y, 8U, 128U, 110U, 40U)) {
                hit = 0;   // DEPLOY
            } else if (mc_ui_touch_in_rect(tg.x, tg.y, 126U, 128U, 110U, 40U)) {
                hit = 1;   // STOW
            } else if (mc_ui_touch_in_rect(tg.x, tg.y, 244U, 128U, 110U, 40U)) {
                hit = 2;   // TEST
            } else if (mc_ui_touch_in_rect(tg.x, tg.y, 362U, 128U, 110U, 40U)) {
                hit = 3;   // ALM CLR
            }

            if (hit >= 0) {
                mc_ui_touch_accept(static_cast<uint32_t>(70 + hit));
                if (static_cast<uint8_t>(hit) != g_sen_sel) {
                    apply_sen_selection(static_cast<uint8_t>(hit));
                }
                touch_enter = true;
            }
        }
    }

    const int32_t clicks = mc_encoder_take_clicks();
    if (clicks != 0) {
        int32_t new_idx = static_cast<int32_t>(g_sen_sel) + clicks;
        while (new_idx < 0) {
            new_idx += 5;
        }
        while (new_idx >= 5) {
            new_idx %= 5;
        }
        const uint8_t target_sel = static_cast<uint8_t>(new_idx);
        if (target_sel != g_sen_sel) {
            apply_sen_selection(target_sel);
        }
    }

    const bool enc_enter = mc_encoder_take_press();
    if (touch_enter || enc_enter) {
        if (g_sen_sel == 4U) { // BACK
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return SensorsResult::Back;
        } else {
            mc_dbg_345_sen_enter_cnt = mc_dbg_345_sen_enter_cnt + 1U;
            mc_dbg_346_sen_enter_idx = g_sen_sel;
            sen_run_action(g_sen_sel);
        }
    }

    sen_msg_poll();

    const uint8_t cycle = g_sen_live_cycle;
    g_sen_live_cycle = static_cast<uint8_t>((g_sen_live_cycle + 1U) % 5U);

    if (cycle <= 2U) { // Endstops X, Y, Z
        const uint8_t axis = cycle;
        const bool hit = read_endstop_hit(axis);
        if (hit != g_cached_endstop_hit[axis]) {
            draw_endstop_card(axis);
        }
    } else if (cycle == 3U) { // BLTouch badge
        const bool busy = read_bltouch_busy();
        const uint8_t pin_code = read_bltouch_pin_code();
        if ((busy != g_cached_bltouch_busy) || (pin_code != g_cached_bltouch_pin)) {
            draw_bltouch_badge();
        }
    } else if (cycle == 4U) { // Filament row
        const bool empty = read_filament_empty();
        if (empty != g_cached_fil_empty) {
            draw_filament_row();
        }
    }

    return SensorsResult::Stay;
}

}  // namespace mc
