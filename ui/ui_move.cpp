// صفحه حرکت رابط کاربری (بند ۸.۸).
// هیچ هدر ST اینجا include نمی‌شود. فقط از hal و gfx و encoder و machine_state استفاده می‌کند.
#include "ui_move.h"
#include "ui_home.h"
#include "ui_colors.h"
#include "ui_touch.h"
#include "../core/gfx.h"
#include "../core/text_format.h"
#include "../core/machine_state.h"
#include "../core/encoder.h"
#include "../core/error.h"
#include "../core/command_queue.h"
#include "../core/gcode_parser.h"
#include "../core/motion_modes.h"
#include "../core/soft_limits.h"
#include "../gcode/gcode_dispatch.h"
#include "../hal/hal_display.h"
#include "../hal/hal_stepper.h"
#include "../hal/hal_buzzer.h"
#include "../hal/hal_time.h"
#include "../config/mc_config.h"

extern "C" {
extern volatile uint32_t mc_dbg_80_mv_arm;

volatile uint32_t mc_dbg_329_move_sel_shown  = 12;  // خواندنی: اندیس مورد فوکوس‌شده (۰ تا ۱۲)
volatile uint32_t mc_dbg_330_move_enter_cnt  = 0;   // خواندنی: تعداد اینترها روی مورد غیر BACK و غیر STEP
volatile uint32_t mc_dbg_331_move_enter_idx  = 255; // خواندنی: اندیس مورد آخرین اینتر (۲۵۵ = هنوز نشده)
volatile uint32_t mc_dbg_332_move_force      = 0;   // نوشتنی: ۰ = بی‌اثر؛ ۱ = باز کردن صفحه حرکت؛ ۲ = برگشت به منو؛ ۳ تا ۱۵ = فوکوس روی مورد (مقدار - ۳)
volatile uint32_t mc_dbg_333_move_step_shown = 1;   // خواندنی: گام نمایشی: ۰ = ۰.۱، ۱ = ۱ (پیش‌فرض)، ۲ = ۱۰
volatile uint32_t mc_dbg_395_mv_act_cnt        = 0;   // خواندنی: تعداد دفعاتی که دکمه‌ها واقعاً کاری انجام دادند
volatile uint32_t mc_dbg_396_mv_act_last       = 255; // خواندنی: اندیس آخرین دکمه موفق (0 تا 11؛ 255 = هنوز نشده)
volatile uint32_t mc_dbg_397_mv_act_refused    = 0;   // خواندنی: تعداد دفعاتی که دکمه رد شد
volatile uint32_t mc_dbg_398_mv_act_en         = 1;   // نوشتنی: 0 = دکمه‌ها هیچ کاری نمی‌کنند (پیش‌فرض 1)
volatile uint32_t mc_dbg_399_mv_refuse_reason  = 0;   // خواندنی: علت آخرین رد شدن
volatile int32_t  mc_dbg_400_mv_jog_last_um    = 0;   // خواندنی: آخرین حرکت دکمه‌های ± محورها، علامت‌دار، به میکرومتر
}

namespace mc {
namespace {

constexpr uint16_t kSelBg         = mc_rgb565(24, 38, 64);
constexpr uint16_t kAxisX         = mc_rgb565(240, 90, 90);
constexpr uint16_t kAxisY         = mc_rgb565(90, 200, 110);
constexpr uint16_t kAxisZ         = mc_rgb565(90, 150, 255);
constexpr uint16_t kAxisOff       = mc_rgb565(70, 80, 100);
constexpr uint16_t kWarn          = mc_rgb565(255, 170, 0);
constexpr uint16_t kWarnFocusBg   = mc_rgb565(60, 40, 8);

static uint8_t g_move_sel        = 12U; // فوکوس اولیه روی BACK (اندیس ۱۲)
static uint8_t g_move_step_idx   = 1U;  // گام نمایشی پیش‌فرض (۱ = 1 mm)
static uint8_t g_move_step       = 0U;  // گام نقاشی ورود (۰ تا ۶، ۲۵۵ = کار عادی)
static uint8_t g_move_axis_cycle = 0U;  // چرخه‌گیری به‌روزرسانی ۳ محور در حالت عادی (۰ تا ۲)

static bool     g_msg_shown    = false;   // پیام علت رد شدن به‌جای عنوان نوار بالا نمایش داده می‌شود
static uint32_t g_msg_start_ms = 0U;      // زمان شروع نمایش پیام

static int32_t g_cached_pos_x10[3] = { -99999, -99999, -99999 };
static bool    g_cached_axis_en[3]  = { false, false, false };

static bool is_move_axis_enabled(uint8_t axis_idx) {
    const bool fake = (mc_dbg_297_ui_fake_on != 0U);
    if (fake) {
        return (mc_dbg_305_ui_fake_axis_en & (1U << axis_idx)) != 0U;
    }
    return mc_stepper_is_enabled(static_cast<StepAxis>(axis_idx));
}

static void draw_axis_row_info(uint8_t r) {
    if (r >= 3U) { return; }
    const uint16_t ry = static_cast<uint16_t>(46U + r * 48U);
    const bool enabled = is_move_axis_enabled(r);

    static const char* const kAxisLetters[3] = { "X", "Y", "Z" };
    static const uint16_t kAxisColors[3]     = { kAxisX, kAxisY, kAxisZ };
    const uint16_t letter_color = enabled ? kAxisColors[r] : kAxisOff;
    mc_gfx_text(20U, static_cast<uint16_t>(ry + 10U), kAxisLetters[r], letter_color, kScreenBg, 3U);

    mc_gfx_fill_rect(60U, static_cast<uint16_t>(ry + 8U), 120U, 16U, kScreenBg);

    const float pos_val = mc_position_get(r);
    char pos_str[16];
    mc_fmt_float(pos_str, sizeof(pos_str), 0U, pos_val, 1U);
    mc_gfx_text(60U, static_cast<uint16_t>(ry + 8U), pos_str, kColorWhite, kScreenBg, 2U);

    mc_gfx_text(60U, static_cast<uint16_t>(ry + 28U), "mm", kLabelGray, kScreenBg, 1U);

    mc_gfx_hline(14U, static_cast<uint16_t>(ry + 46U), 452U, kDividerGray);

    g_cached_pos_x10[r] = static_cast<int32_t>(pos_val >= 0.0f ? (pos_val * 10.0f + 0.5f) : (pos_val * 10.0f - 0.5f));
    g_cached_axis_en[r] = enabled;
}

static void draw_move_item(uint8_t item_idx) {
    if (item_idx >= 13U) { return; }
    const bool focused = (item_idx == g_move_sel);

    if (item_idx <= 8U) {
        const uint8_t r = item_idx / 3U;
        const uint8_t btn_type = item_idx % 3U;
        const uint16_t ry = static_cast<uint16_t>(46U + r * 48U);
        const uint16_t by = static_cast<uint16_t>(ry + 6U);

        uint16_t bx = 190U;
        uint16_t bw = 72U;
        if (btn_type == 1U) {
            bx = 270U;
            bw = 72U;
        } else if (btn_type == 2U) {
            bx = 350U;
            bw = 114U;
        }
        constexpr uint16_t bh = 32U;

        const uint16_t bg_col     = focused ? kSelBg : kCardBg;
        const uint16_t fg_col     = focused ? kColorWhite : kLabelGray;
        const uint16_t border_col = focused ? kAccentLine : kCardBorder;

        mc_gfx_fill_rect(bx, by, bw, bh, bg_col);

        mc_gfx_rect(bx, by, bw, bh, border_col);
        if (focused) {
            mc_gfx_rect(static_cast<uint16_t>(bx + 1U), static_cast<uint16_t>(by + 1U),
                        static_cast<uint16_t>(bw - 2U), static_cast<uint16_t>(bh - 2U), border_col);
        }

        if (btn_type == 0U) { // "-"
            const uint16_t cx = static_cast<uint16_t>(bx + (bw - 16U) / 2U);
            const uint16_t cy = static_cast<uint16_t>(by + (bh - 3U) / 2U);
            mc_gfx_fill_rect(cx, cy, 16U, 3U, fg_col);
        } else if (btn_type == 1U) { // "+"
            const uint16_t cx_h = static_cast<uint16_t>(bx + (bw - 16U) / 2U);
            const uint16_t cy_h = static_cast<uint16_t>(by + (bh - 3U) / 2U);
            mc_gfx_fill_rect(cx_h, cy_h, 16U, 3U, fg_col);

            const uint16_t cx_v = static_cast<uint16_t>(bx + (bw - 3U) / 2U);
            const uint16_t cy_v = static_cast<uint16_t>(by + (bh - 16U) / 2U);
            mc_gfx_fill_rect(cx_v, cy_v, 3U, 16U, fg_col);
        } else { // "HOME"
            const uint16_t tw = mc_gfx_text_width("HOME", 2U);
            const uint16_t tx = static_cast<uint16_t>(bx + (bw - tw) / 2U);
            const uint16_t ty = static_cast<uint16_t>(by + (bh - 16U) / 2U);
            mc_gfx_text(tx, ty, "HOME", fg_col, bg_col, 2U);
        }
    } else if (item_idx == 9U) { // STEP
        const uint16_t bg_row = focused ? kSelBg : kScreenBg;
        mc_gfx_fill_rect(8U, 194U, 464U, 36U, bg_row);

        if (focused) {
            mc_gfx_fill_rect(8U, 194U, 4U, 36U, kAccentLine);
        }

        const uint16_t label_col = focused ? kColorWhite : kLabelGray;
        mc_gfx_text(20U, 204U, "STEP", label_col, bg_row, 2U);
        mc_gfx_text(430U, 206U, "mm", kLabelGray, bg_row, 1U);

        static const char* const kChipTexts[3] = { "0.1", "1", "10" };
        static const uint16_t kChipX[3]        = { 190U, 270U, 350U };

        for (uint8_t c = 0U; c < 3U; ++c) {
            const bool is_active = (c == g_move_step_idx);
            const uint16_t cx = kChipX[c];
            constexpr uint16_t cy = 198U;
            constexpr uint16_t cw = 72U;
            constexpr uint16_t ch = 28U;

            if (is_active) {
                mc_gfx_fill_rect(cx, cy, cw, ch, kAccentLine);
                const uint16_t tw = mc_gfx_text_width(kChipTexts[c], 2U);
                const uint16_t tx = static_cast<uint16_t>(cx + (cw - tw) / 2U);
                const uint16_t ty = static_cast<uint16_t>(cy + (ch - 16U) / 2U);
                mc_gfx_text(tx, ty, kChipTexts[c], kScreenBg, kAccentLine, 2U);
            } else {
                mc_gfx_fill_rect(cx, cy, cw, ch, kCardBg);
                mc_gfx_rect(cx, cy, cw, ch, kCardBorder);
                const uint16_t tw = mc_gfx_text_width(kChipTexts[c], 2U);
                const uint16_t tx = static_cast<uint16_t>(cx + (cw - tw) / 2U);
                const uint16_t ty = static_cast<uint16_t>(cy + (ch - 16U) / 2U);
                mc_gfx_text(tx, ty, kChipTexts[c], kLabelGray, kCardBg, 2U);
            }
        }
    } else if (item_idx == 10U) { // HOME ALL
        constexpr uint16_t bx = 8U;
        constexpr uint16_t by = 244U;
        constexpr uint16_t bw = 150U;
        constexpr uint16_t bh = 52U;

        const uint16_t bg_col     = focused ? kSelBg : kCardBg;
        const uint16_t fg_col     = focused ? kColorWhite : kLabelGray;
        const uint16_t border_col = focused ? kAccentLine : kCardBorder;

        mc_gfx_fill_rect(bx, by, bw, bh, bg_col);
        mc_gfx_rect(bx, by, bw, bh, border_col);
        if (focused) {
            mc_gfx_rect(static_cast<uint16_t>(bx + 1U), static_cast<uint16_t>(by + 1U),
                        static_cast<uint16_t>(bw - 2U), static_cast<uint16_t>(bh - 2U), border_col);
        }

        const uint16_t tw = mc_gfx_text_width("HOME ALL", 2U);
        const uint16_t tx = static_cast<uint16_t>(bx + (bw - tw) / 2U);
        const uint16_t ty = static_cast<uint16_t>(by + (bh - 16U) / 2U);
        mc_gfx_text(tx, ty, "HOME ALL", fg_col, bg_col, 2U);
    } else if (item_idx == 11U) { // MOTORS OFF
        constexpr uint16_t bx = 166U;
        constexpr uint16_t by = 244U;
        constexpr uint16_t bw = 150U;
        constexpr uint16_t bh = 52U;

        const uint16_t bg_col     = focused ? kWarnFocusBg : kCardBg;
        const uint16_t fg_col     = focused ? kColorWhite : kWarn;
        const uint16_t border_col = kWarn;

        mc_gfx_fill_rect(bx, by, bw, bh, bg_col);
        mc_gfx_rect(bx, by, bw, bh, border_col);
        if (focused) {
            mc_gfx_rect(static_cast<uint16_t>(bx + 1U), static_cast<uint16_t>(by + 1U),
                        static_cast<uint16_t>(bw - 2U), static_cast<uint16_t>(bh - 2U), border_col);
        }

        const uint16_t tw = mc_gfx_text_width("MOTORS OFF", 2U);
        const uint16_t tx = static_cast<uint16_t>(bx + (bw - tw) / 2U);
        const uint16_t ty = static_cast<uint16_t>(by + (bh - 16U) / 2U);
        mc_gfx_text(tx, ty, "MOTORS OFF", fg_col, bg_col, 2U);
    } else if (item_idx == 12U) { // BACK
        constexpr uint16_t bx = 324U;
        constexpr uint16_t by = 244U;
        constexpr uint16_t bw = 148U;
        constexpr uint16_t bh = 52U;

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

static void apply_move_selection(uint8_t target_sel) {
    if (target_sel >= 13U) { return; }
    const uint8_t old_sel = g_move_sel;
    g_move_sel = target_sel;
    mc_dbg_329_move_sel_shown = g_move_sel;

    if (old_sel != g_move_sel) {
        draw_move_item(old_sel);
        draw_move_item(g_move_sel);
    }
}

// گام هر چیپ STEP به دهم میلی‌متر (0.1 و 1 و 10 میلی‌متر). با kChipTexts در draw_move_item هماهنگ است.
static const int32_t kMoveStepX10[3] = { 1, 10, 100 };

// بند ۸.۱۶: پیام علت رد شدن به‌جای عنوان نوار بالا و بوق بلند
static void move_refuse_feedback(uint32_t reason) {
    const char* txt = "REFUSED";
    switch (reason) {
        case 2U: txt = "KILLED";             break;
        case 3U: txt = "FAKE MODE ON";       break;
        case 4U: txt = "BUTTONS DISABLED";   break;
        case 6U: txt = "MOTION NOT ARMED";   break;
        case 7U: txt = "WAIT: JOB RUNNING";  break;
        case 8U: txt = "QUEUE ERROR";        break;
        case 9U: txt = "SOFT LIMIT";         break;
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

// بعد از MC_UI_MOVE_MSG_MS عنوان MOVE را برمی‌گرداند
static void move_msg_poll() {
    if (g_msg_shown && ((mc_millis() - g_msg_start_ms) >= static_cast<uint32_t>(MC_UI_MOVE_MSG_MS))) {
        mc_gfx_fill_rect(0U, 0U, 480U, 36U, kTopBarBg);
        const uint16_t tw = mc_gfx_text_width("MOVE", 2U);
        const uint16_t tx = static_cast<uint16_t>((480U - tw) / 2U);
        mc_gfx_text(tx, 10U, "MOVE", kColorWhite, kTopBarBg, 2U);
        g_msg_shown = false;
    }
}

static void move_note_refuse(uint32_t reason) {
    mc_dbg_397_mv_act_refused   = mc_dbg_397_mv_act_refused + 1U;
    mc_dbg_399_mv_refuse_reason = reason;
    move_refuse_feedback(reason);
}

static void move_note_ok(uint8_t item_idx) {
    mc_dbg_395_mv_act_cnt       = mc_dbg_395_mv_act_cnt + 1U;
    mc_dbg_396_mv_act_last      = item_idx;
    mc_dbg_399_mv_refuse_reason = 0U;
}

// یک خط G-code را تجزیه می‌کند و به صف دستورها می‌دهد. false = شکست
static bool move_push_line(const char* line) {
    static GcodeCommand cmd;   // static چون حدود 240 بایت است
    if (mc_gcode_parse(line, cmd) != GcodeResult::Ok) { return false; }
    return mc_cmdq_push(cmd);
}

// آیا کار قبلی (صف دستور، اجرای دستور، حرکت یا هومینگ) هنوز تمام نشده؟
static bool move_job_busy() {
    return (!mc_cmdq_empty()) || mc_dispatch_busy() || mc_motion_busy();
}

static void move_run_action(uint8_t item_idx) {
    const bool is_off  = (item_idx == 11U);
    const bool is_all  = (item_idx == 10U);
    const bool is_jog  = (item_idx <= 8U) && ((item_idx % 3U) != 2U);
    const bool is_home = is_all || ((item_idx <= 8U) && ((item_idx % 3U) == 2U));
    if (!(is_off || is_all || is_jog || is_home)) { return; }   // اندیس 9 و 12 و بزرگ‌تر: کاری ندارند

    // ۱) شرط‌های رد شدن، به این ترتیب (اولین شرط برقرار، دکمه را رد می‌کند)
    // MOTORS OFF عمل ایمن است: kill و arm برایش بررسی نمی‌شود.
    if ((!is_off) && mc_is_killed())            { move_note_refuse(2U); return; }
    if (mc_dbg_297_ui_fake_on != 0U)            { move_note_refuse(3U); return; }
    if (mc_dbg_398_mv_act_en == 0U)             { move_note_refuse(4U); return; }
    if ((!is_off) && (mc_dbg_80_mv_arm != 1U))  { move_note_refuse(6U); return; }
    if (move_job_busy())                        { move_note_refuse(7U); return; }

    char line[48];

    // ۲) MOTORS OFF
    if (is_off) {
        (void)mc_fmt_text(line, sizeof(line), 0U, "M84");
        if (!move_push_line(line)) { move_note_refuse(8U); return; }
        move_note_ok(item_idx);
        return;
    }

    static const char kLetters[3] = { 'X', 'Y', 'Z' };

    // ۳) HOME ALL و HOME هر محور (فقط از مسیر G28؛ ایمنی هومینگ را خود G28 دارد)
    if (is_home) {
        uint16_t p = mc_fmt_text(line, sizeof(line), 0U, "G28");
        if (!is_all) {
            const char ax[2] = { kLetters[item_idx / 3U], '\0' };
            p = mc_fmt_text(line, sizeof(line), p, " ");
            p = mc_fmt_text(line, sizeof(line), p, ax);
        }
        (void)p;
        if (!move_push_line(line)) { move_note_refuse(8U); return; }
        move_note_ok(item_idx);
        return;
    }

    // ۴) حرکت ± یک محور. مستقل از G90/G91: در حالت مطلق، مقصد = موقعیت فعلی + گام
    const uint8_t axis = static_cast<uint8_t>(item_idx / 3U);
    const bool    plus = ((item_idx % 3U) == 1U);
    const int32_t step_x10 = kMoveStepX10[g_move_step_idx % 3U];
    const float   delta  = (plus ? 1.0f : -1.0f) * (static_cast<float>(step_x10) / 10.0f);
    const float   target = mc_position_get(axis) + delta;

    // پیش‌بررسی حد نرم‌افزاری تا خطا همین‌جا و روی صفحه دیده شود
    if (!mc_softlimit_allows(axis, target)) {
        mc_softlimit_note_refused();
        move_note_refuse(9U);
        return;
    }

    const float value = mc_mode_relative_get() ? delta : target;
    uint32_t feed = static_cast<uint32_t>(MC_UI_MOVE_FEED_X_MM_MIN);
    if (axis == 1U) {
        feed = static_cast<uint32_t>(MC_UI_MOVE_FEED_Y_MM_MIN);
    } else if (axis == 2U) {
        feed = static_cast<uint32_t>(MC_UI_MOVE_FEED_Z_MM_MIN);
    }
    const float old_feed = mc_mode_feed_get_mmin();

    // خط اول: حرکت (مثل G1 X1.000 F3000). خط دوم: برگرداندن فید قبلی دستگاه (G1 F1800.00)
    {
        const char ax[2] = { kLetters[axis], '\0' };
        uint16_t p = mc_fmt_text(line, sizeof(line), 0U, "G1 ");
        p = mc_fmt_text(line, sizeof(line), p, ax);
        p = mc_fmt_float(line, sizeof(line), p, value, 3U);
        p = mc_fmt_text(line, sizeof(line), p, " F");
        p = mc_fmt_uint(line, sizeof(line), p, feed);
        (void)p;
    }
    if (!move_push_line(line)) { move_note_refuse(8U); return; }
    {
        uint16_t p = mc_fmt_text(line, sizeof(line), 0U, "G1 F");
        p = mc_fmt_float(line, sizeof(line), p, old_feed, 2U);
        (void)p;
    }
    if (!move_push_line(line)) { move_note_refuse(8U); return; }

    mc_dbg_400_mv_jog_last_um = (plus ? 1 : -1) * step_x10 * 100;
    move_note_ok(item_idx);
}

}  // namespace

void mc_ui_move_open() {
    g_move_sel        = 12U;
    g_move_step_idx   = 1U;
    g_move_step       = 0U;
    g_move_axis_cycle = 0U;
    g_msg_shown       = false;
    g_msg_start_ms    = 0U;

    g_cached_pos_x10[0] = -99999;
    g_cached_pos_x10[1] = -99999;
    g_cached_pos_x10[2] = -99999;
    g_cached_axis_en[0] = false;
    g_cached_axis_en[1] = false;
    g_cached_axis_en[2] = false;

    mc_dbg_329_move_sel_shown  = 12U;
    mc_dbg_333_move_step_shown = 1U;

    (void)mc_encoder_take_clicks();
    (void)mc_encoder_take_press();
    mc_ui_touch_reset();
}

MoveResult mc_ui_move_poll() {
    (void)mc_dbg_329_move_sel_shown;
    (void)mc_dbg_330_move_enter_cnt;
    (void)mc_dbg_331_move_enter_idx;
    (void)mc_dbg_332_move_force;
    (void)mc_dbg_333_move_step_shown;
    (void)mc_dbg_395_mv_act_cnt;
    (void)mc_dbg_396_mv_act_last;
    (void)mc_dbg_397_mv_act_refused;
    (void)mc_dbg_398_mv_act_en;
    (void)mc_dbg_399_mv_refuse_reason;
    (void)mc_dbg_400_mv_jog_last_um;

    if (mc_dbg_332_move_force != 0U) {
        const uint32_t force_val = mc_dbg_332_move_force;
        mc_dbg_332_move_force = 0U;

        if (force_val == 2U) {
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return MoveResult::Back;
        } else if ((force_val >= 3U) && (force_val <= 15U)) {
            const uint8_t target_sel = static_cast<uint8_t>(force_val - 3U);
            if (target_sel != g_move_sel) {
                apply_move_selection(target_sel);
            }
        }
    }

    if (g_move_step < 255U) {
        switch (g_move_step) {
            case 0:
                mc_display_fill_screen(kScreenBg);
                g_move_step = 1U;
                return MoveResult::Stay;

            case 1: {
                mc_gfx_fill_rect(0U, 0U, 480U, 36U, kTopBarBg);
                mc_gfx_fill_rect(0U, 36U, 480U, 2U, kAccentLine);
                const uint16_t tw = mc_gfx_text_width("MOVE", 2U);
                const uint16_t tx = static_cast<uint16_t>((480U - tw) / 2U);
                mc_gfx_text(tx, 10U, "MOVE", kColorWhite, kTopBarBg, 2U);
                g_move_step = 2U;
                return MoveResult::Stay;
            }

            case 2:
                draw_axis_row_info(0U);
                draw_move_item(0U);
                draw_move_item(1U);
                draw_move_item(2U);
                g_move_step = 3U;
                return MoveResult::Stay;

            case 3:
                draw_axis_row_info(1U);
                draw_move_item(3U);
                draw_move_item(4U);
                draw_move_item(5U);
                g_move_step = 4U;
                return MoveResult::Stay;

            case 4:
                draw_axis_row_info(2U);
                draw_move_item(6U);
                draw_move_item(7U);
                draw_move_item(8U);
                g_move_step = 5U;
                return MoveResult::Stay;

            case 5:
                draw_move_item(9U);
                g_move_step = 6U;
                return MoveResult::Stay;

            case 6:
                draw_move_item(10U);
                draw_move_item(11U);
                draw_move_item(12U);
                g_move_step = 255U;
                return MoveResult::Stay;

            default:
                g_move_step = 255U;
                return MoveResult::Stay;
        }
    }

    bool touch_enter = false;
    // لمس (بند ۸.۱۶): ضربه روی دکمه‌های صفحه حرکت
    {
        const TouchGesture tg = mc_ui_touch_poll();
        if (tg.evt == TouchEvt::Tap) {
            if (mc_ui_touch_in_rect(tg.x, tg.y, 324U, 244U, 148U, 52U)) {
                mc_ui_touch_accept(20U);
                (void)mc_encoder_take_clicks();
                (void)mc_encoder_take_press();
                return MoveResult::Back;
            }

            int32_t hit = -1;   // اندیس دکمه عملیاتی (0 تا 8 و 10 و 11)
            for (uint8_t i = 0U; i < 9U; ++i) {
                const uint8_t  r  = static_cast<uint8_t>(i / 3U);
                const uint8_t  t  = static_cast<uint8_t>(i % 3U);
                const uint16_t by = static_cast<uint16_t>(52U + r * 48U);
                const uint16_t bx = (t == 0U) ? 190U : ((t == 1U) ? 270U : 350U);
                const uint16_t bw = (t == 2U) ? 114U : 72U;
                if (mc_ui_touch_in_rect(tg.x, tg.y, bx, by, bw, 32U)) {
                    hit = static_cast<int32_t>(i);
                    break;
                }
            }
            if (hit < 0) {
                if (mc_ui_touch_in_rect(tg.x, tg.y, 8U, 244U, 150U, 52U)) {
                    hit = 10;   // HOME ALL
                } else if (mc_ui_touch_in_rect(tg.x, tg.y, 166U, 244U, 150U, 52U)) {
                    hit = 11;   // MOTORS OFF
                }
            }

            if (hit >= 0) {
                uint32_t id = static_cast<uint32_t>(50 + hit);   // 50 تا 58
                if (hit == 10) { id = 63U; }
                if (hit == 11) { id = 64U; }
                mc_ui_touch_accept(id);
                if (static_cast<uint8_t>(hit) != g_move_sel) {
                    apply_move_selection(static_cast<uint8_t>(hit));
                }
                touch_enter = true;
            } else {
                // چیپ‌های گام: ضربه روی هر چیپ آن را مستقیم فعال می‌کند (اینتر نمی‌زند)
                static const uint16_t kChipTapX[3] = { 190U, 270U, 350U };
                for (uint8_t c = 0U; c < 3U; ++c) {
                    if (mc_ui_touch_in_rect(tg.x, tg.y, kChipTapX[c], 198U, 72U, 28U)) {
                        mc_ui_touch_accept(static_cast<uint32_t>(60U + c));
                        g_move_step_idx = c;
                        mc_dbg_333_move_step_shown = g_move_step_idx;
                        if (g_move_sel != 9U) {
                            apply_move_selection(9U);
                        }
                        draw_move_item(9U);
                        break;
                    }
                }
            }
        }
    }

    const int32_t clicks = mc_encoder_take_clicks();
    if (clicks != 0) {
        int32_t new_idx = static_cast<int32_t>(g_move_sel) + clicks;
        while (new_idx < 0) {
            new_idx += 13;
        }
        while (new_idx >= 13) {
            new_idx %= 13;
        }
        const uint8_t target_sel = static_cast<uint8_t>(new_idx);
        if (target_sel != g_move_sel) {
            apply_move_selection(target_sel);
        }
    }

    const bool enc_enter = mc_encoder_take_press();
    if (touch_enter || enc_enter) {
        if (g_move_sel == 12U) {
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return MoveResult::Back;
        } else if (g_move_sel == 9U) {
            g_move_step_idx = static_cast<uint8_t>((g_move_step_idx + 1U) % 3U);
            mc_dbg_333_move_step_shown = g_move_step_idx;
            draw_move_item(9U);
        } else {
            mc_dbg_330_move_enter_cnt = mc_dbg_330_move_enter_cnt + 1U;
            mc_dbg_331_move_enter_idx = g_move_sel;
            move_run_action(g_move_sel);
        }
    }

    move_msg_poll();

    const uint8_t axis_r = g_move_axis_cycle;
    g_move_axis_cycle = static_cast<uint8_t>((g_move_axis_cycle + 1U) % 3U);

    const bool curr_en = is_move_axis_enabled(axis_r);
    const float curr_pos = mc_position_get(axis_r);
    const int32_t curr_pos_x10 = static_cast<int32_t>(curr_pos >= 0.0f ? (curr_pos * 10.0f + 0.5f) : (curr_pos * 10.0f - 0.5f));

    if ((curr_en != g_cached_axis_en[axis_r]) || (curr_pos_x10 != g_cached_pos_x10[axis_r])) {
        draw_axis_row_info(axis_r);
    }

    return MoveResult::Stay;
}

}  // namespace mc
