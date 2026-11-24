// صفحه DISPLAY رابط کاربری (بند ۸.۲۱).
// هیچ هدر ST اینجا include نمی‌شود. فقط از hal و gfx و encoder و disp_settings استفاده می‌کند.
#include "ui_display.h"
#include "ui_colors.h"
#include "ui_touch.h"
#include "../core/gfx.h"
#include "../core/encoder.h"
#include "../core/disp_settings.h"
#include "../hal/hal_display.h"
#include "../hal/hal_time.h"
#include "../config/mc_config.h"

extern "C" {
volatile uint32_t mc_dbg_424_disp_force        = 0;  // نوشتنی: 0 = بی‌اثر؛ 1 = باز کردن صفحه؛ 2 = برگشت به منو
volatile uint32_t mc_dbg_425_disp_sel_shown    = 0;  // خواندنی: مورد فوکوس‌شده (0 = SCREEN OFF، 1 = MENU EXIT، 2 = BACK)
volatile uint32_t mc_dbg_426_disp_scr_off_s    = 0;  // خواندنی: زمان خاموشی نمایشگر به ثانیه (0 = هیچ‌وقت)
volatile uint32_t mc_dbg_427_disp_menu_exit_s  = 0;  // خواندنی: زمان خروج خودکار از منو به ثانیه (0 = هیچ‌وقت)
volatile uint32_t mc_dbg_430_disp_store_state  = 0;  // خواندنی: وضعیت ذخیره (0 بیکار، 1 در جریان، 2 موفق، 3 خطا)
volatile uint32_t mc_dbg_431_disp_store_result = 0;  // خواندنی: کد نتیجه ذخیره یا بارگذاری
volatile uint32_t mc_dbg_432_disp_boot_src     = 0;  // خواندنی: منبع مقدارها هنگام بوت (0 پیش‌فرض، 1 EEPROM، 2 خطا)
volatile uint32_t mc_dbg_433_disp_clear_force  = 0;  // نوشتنی: 1 = رکورد EEPROM نامعتبر شود (برای تست)
}

namespace mc {
namespace {

constexpr uint16_t kSelBg   = mc_rgb565(24, 38, 64);
constexpr uint16_t kAccDisp = mc_rgb565(255, 90, 160);   // رنگ شاخص DISPLAY (همان رنگ آیتم DISPLAY در منو)

// گزینه‌های هر تنظیم (ثانیه) و متن نمایشی هرکدام. مقدار 0 = هیچ‌وقت.
constexpr uint8_t  kOptCount = 7U;
constexpr uint32_t kScrOptS[kOptCount]  = { 0U, 15U, 30U, 60U, 120U, 300U, 600U };
constexpr uint32_t kMenuOptS[kOptCount] = { 0U, 10U, 15U, 30U, 60U, 120U, 300U };
const char* const kScrOptTxt[kOptCount]  = { "NEVER", "15 S", "30 S", "1 MIN", "2 MIN", "5 MIN", "10 MIN" };
const char* const kMenuOptTxt[kOptCount] = { "NEVER", "10 S", "15 S", "30 S", "1 MIN", "2 MIN", "5 MIN" };

// مختصات (پیکسل)
constexpr uint16_t kCardX  = 8U;
constexpr uint16_t kCardW  = 464U;
constexpr uint16_t kCardH  = 60U;
constexpr uint16_t kRow0Y  = 50U;
constexpr uint16_t kRow1Y  = 118U;
constexpr uint16_t kMinusX = 336U;
constexpr uint16_t kPlusX  = 404U;
constexpr uint16_t kBtnW   = 56U;
constexpr uint16_t kBtnH   = 44U;
constexpr uint16_t kBackX  = 324U;
constexpr uint16_t kBackY  = 258U;
constexpr uint16_t kBackW  = 148U;
constexpr uint16_t kBackH  = 44U;

uint8_t  g_step     = 0U;               // گام نقاشی ورود (۰ تا ۵، ۲۵۵ = کار عادی)
uint8_t  g_sel      = 0U;               // مورد فوکوس‌شده: 0 و 1 = دو تنظیم، 2 = BACK
bool     g_edit     = false;            // حالت ویرایش با انکودر
uint8_t  g_idx[2]   = { 3U, 3U };       // اندیس گزینه فعلی هر تنظیم
bool     g_dirty    = false;            // تغییر ذخیره‌نشده داریم
uint32_t g_dirty_ms = 0U;               // زمان آخرین تغییر

uint16_t row_y(uint8_t r) {
    return (r == 0U) ? kRow0Y : kRow1Y;
}

// اندیس نزدیک‌ترین گزینه (اولین گزینه‌ای که کمتر از مقدار فعلی نباشد)
uint8_t nearest_index(const uint32_t* opts, uint32_t cur) {
    if (cur == 0U) { return 0U; }
    for (uint8_t i = 1U; i < kOptCount; ++i) {
        if (opts[i] >= cur) { return i; }
    }
    return static_cast<uint8_t>(kOptCount - 1U);
}

void draw_top_bar() {
    mc_gfx_fill_rect(0U, 0U, 480U, 36U, kTopBarBg);
    mc_gfx_fill_rect(0U, 36U, 480U, 2U, kAccentLine);
    const uint16_t tw = mc_gfx_text_width("DISPLAY", 2U);
    const uint16_t tx = static_cast<uint16_t>((480U - tw) / 2U);
    mc_gfx_text(tx, 10U, "DISPLAY", kColorWhite, kTopBarBg, 2U);
}

// دکمه کوچک - یا +
void draw_step_button(uint16_t x, uint16_t y, const char* sym, bool active) {
    mc_gfx_fill_rect(x, y, kBtnW, kBtnH, kCardBg);
    mc_gfx_rect(x, y, kBtnW, kBtnH, active ? kAccDisp : kCardBorder);
    const uint16_t tw = mc_gfx_text_width(sym, 2U);
    mc_gfx_text(static_cast<uint16_t>(x + (kBtnW - tw) / 2U),
                static_cast<uint16_t>(y + (kBtnH - 16U) / 2U),
                sym, active ? kColorWhite : kDividerGray, kCardBg, 2U);
}

// رسم یک کارت تنظیم (r = 0 یا 1)
void draw_row(uint8_t r) {
    const uint16_t y = row_y(r);
    const bool focus = (g_sel == r);
    const bool edit  = focus && g_edit;
    const uint16_t bg     = focus ? kSelBg : kCardBg;
    const uint16_t border = edit ? kAccDisp : (focus ? kAccentLine : kCardBorder);

    mc_gfx_fill_rect(kCardX, y, kCardW, kCardH, bg);
    mc_gfx_rect(kCardX, y, kCardW, kCardH, border);
    mc_gfx_fill_rect(kCardX, y, 4U, kCardH, kAccDisp);

    const char* label = (r == 0U) ? "SCREEN OFF AFTER" : "MENU EXIT AFTER";
    const char* value = (r == 0U) ? kScrOptTxt[g_idx[0]] : kMenuOptTxt[g_idx[1]];
    mc_gfx_text(24U, static_cast<uint16_t>(y + 10U), label, kLabelGray, bg, 1U);
    mc_gfx_text(24U, static_cast<uint16_t>(y + 28U), value, kColorWhite, bg, 2U);

    draw_step_button(kMinusX, static_cast<uint16_t>(y + 8U), "-", g_idx[r] > 0U);
    draw_step_button(kPlusX,  static_cast<uint16_t>(y + 8U), "+", g_idx[r] < static_cast<uint8_t>(kOptCount - 1U));
}

void draw_back_button() {
    const bool focus = (g_sel == 2U);
    const uint16_t border = focus ? kAccentLine : kCardBorder;
    mc_gfx_fill_rect(kBackX, kBackY, kBackW, kBackH, kSelBg);
    mc_gfx_rect(kBackX, kBackY, kBackW, kBackH, border);
    if (focus) {
        mc_gfx_rect(static_cast<uint16_t>(kBackX + 1U), static_cast<uint16_t>(kBackY + 1U),
                    static_cast<uint16_t>(kBackW - 2U), static_cast<uint16_t>(kBackH - 2U), border);
    }
    const uint16_t tw = mc_gfx_text_width("BACK", 2U);
    const uint16_t tx = static_cast<uint16_t>(kBackX + (kBackW - tw) / 2U);
    const uint16_t ty = static_cast<uint16_t>(kBackY + (kBackH - 16U) / 2U);
    mc_gfx_text(tx, ty, "BACK", kColorWhite, kSelBg, 2U);
}

// راهنمای دو خطی پایین سمت چپ
void draw_hint() {
    mc_gfx_fill_rect(8U, 258U, 308U, 44U, kScreenBg);
    const char* l1 = g_edit ? "TURN: CHANGE VALUE" : "TURN: SELECT";
    const char* l2 = g_edit ? "PRESS: OK" : "PRESS: EDIT / OPEN";
    mc_gfx_text(16U, 266U, l1, kLabelGray, kScreenBg, 1U);
    mc_gfx_text(16U, 282U, l2, kLabelGray, kScreenBg, 1U);
}

void draw_item(uint8_t idx) {
    if (idx < 2U) {
        draw_row(idx);
    } else {
        draw_back_button();
    }
}

// درخواست ذخیره بدون انتظار: تابع پس‌زمینه در همین دور بعدی ذخیره را شروع می‌کند.
void request_save_now() {
    if (g_dirty) {
        g_dirty_ms = mc_millis() - static_cast<uint32_t>(MC_UI_DISP_SAVE_DELAY_MS);
    }
}

// تغییر فوکوس (و خروج از حالت ویرایش)
void set_focus(uint8_t new_sel) {
    const uint8_t old_sel = g_sel;
    if ((new_sel == old_sel) && (!g_edit)) { return; }
    g_edit = false;
    g_sel = new_sel;
    mc_dbg_425_disp_sel_shown = g_sel;
    draw_item(old_sel);
    draw_item(g_sel);
    draw_hint();
}

// تغییر مقدار تنظیم r به اندازه delta پله. true اگر مقدار واقعاً عوض شد.
bool change_value(uint8_t r, int32_t delta) {
    int32_t ni = static_cast<int32_t>(g_idx[r]) + delta;
    const int32_t kMaxIdx = static_cast<int32_t>(kOptCount) - 1;
    if (ni < 0) { ni = 0; }
    if (ni > kMaxIdx) { ni = kMaxIdx; }
    if (ni == static_cast<int32_t>(g_idx[r])) { return false; }
    g_idx[r] = static_cast<uint8_t>(ni);
    (void)mc_dispset_set_values(kScrOptS[g_idx[0]], kMenuOptS[g_idx[1]]);
    g_dirty = true;
    g_dirty_ms = mc_millis();
    draw_row(r);
    return true;
}

}  // namespace

void mc_ui_display_open() {
    g_step = 0U;
    g_sel  = 0U;
    g_edit = false;
    g_idx[0] = nearest_index(kScrOptS, mc_dispset_screen_off_s());
    g_idx[1] = nearest_index(kMenuOptS, mc_dispset_menu_exit_s());

    mc_dbg_424_disp_force = 0U;
    mc_dbg_425_disp_sel_shown = 0U;

    (void)mc_encoder_take_clicks();
    (void)mc_encoder_take_press();
    mc_ui_touch_reset();
}

DisplayResult mc_ui_display_poll() {
    (void)mc_dbg_424_disp_force;
    (void)mc_dbg_425_disp_sel_shown;

    if (mc_dbg_424_disp_force != 0U) {
        const uint32_t force_val = mc_dbg_424_disp_force;
        mc_dbg_424_disp_force = 0U;
        if (force_val == 2U) {
            request_save_now();
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return DisplayResult::Back;
        } else if (force_val == 1U) {
            g_step = 0U;
        }
    }

    if (g_step < 255U) {
        switch (g_step) {
            case 0:
                mc_display_fill_screen(kScreenBg);
                g_step = 1U;
                return DisplayResult::Stay;
            case 1:
                draw_top_bar();
                g_step = 2U;
                return DisplayResult::Stay;
            case 2:
                draw_row(0U);
                g_step = 3U;
                return DisplayResult::Stay;
            case 3:
                draw_row(1U);
                g_step = 4U;
                return DisplayResult::Stay;
            case 4:
                draw_back_button();
                g_step = 5U;
                return DisplayResult::Stay;
            case 5:
                draw_hint();
                g_step = 255U;
                return DisplayResult::Stay;
            default:
                g_step = 255U;
                return DisplayResult::Stay;
        }
    }

    // لمس (بند ۸.۲۱)
    {
        const TouchGesture tg = mc_ui_touch_poll();
        if (tg.evt == TouchEvt::Tap) {
            if (mc_ui_touch_in_rect(tg.x, tg.y, kBackX, kBackY, kBackW, kBackH)) {
                mc_ui_touch_accept(20U);
                request_save_now();
                (void)mc_encoder_take_clicks();
                (void)mc_encoder_take_press();
                return DisplayResult::Back;
            }
            for (uint8_t r = 0U; r < 2U; ++r) {
                const uint16_t y = row_y(r);
                const uint16_t by = static_cast<uint16_t>(y + 8U);
                if (mc_ui_touch_in_rect(tg.x, tg.y, kMinusX, by, kBtnW, kBtnH)) {
                    mc_ui_touch_accept(static_cast<uint32_t>(80U + 2U * r));
                    set_focus(r);
                    (void)change_value(r, -1);
                    break;
                } else if (mc_ui_touch_in_rect(tg.x, tg.y, kPlusX, by, kBtnW, kBtnH)) {
                    mc_ui_touch_accept(static_cast<uint32_t>(81U + 2U * r));
                    set_focus(r);
                    (void)change_value(r, 1);
                    break;
                } else if (mc_ui_touch_in_rect(tg.x, tg.y, kCardX, y,
                                               static_cast<uint16_t>(kMinusX - kCardX - 4U), kCardH)) {
                    mc_ui_touch_accept(static_cast<uint32_t>(84U + r));
                    set_focus(r);
                    break;
                }
            }
        }
    }

    // چرخش انکودر
    const int32_t clicks = mc_encoder_take_clicks();
    if (clicks != 0) {
        if (g_edit && (g_sel < 2U)) {
            (void)change_value(g_sel, clicks);
        } else {
            int32_t n = static_cast<int32_t>(g_sel) + clicks;
            n %= 3;
            if (n < 0) { n += 3; }
            set_focus(static_cast<uint8_t>(n));
        }
    }

    // فشردن انکودر
    if (mc_encoder_take_press()) {
        if (g_sel == 2U) {
            request_save_now();
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return DisplayResult::Back;
        }
        g_edit = !g_edit;
        draw_row(g_sel);
        draw_hint();
        if (!g_edit) {
            request_save_now();   // تأیید ویرایش: ذخیره فوری
        }
    }

    return DisplayResult::Stay;
}

void mc_ui_display_bg_poll() {
    (void)mc_dbg_432_disp_boot_src;

    mc_dbg_426_disp_scr_off_s    = mc_dispset_screen_off_s();
    mc_dbg_427_disp_menu_exit_s  = mc_dispset_menu_exit_s();
    mc_dbg_430_disp_store_state  = mc_dispset_state();
    mc_dbg_431_disp_store_result = mc_dispset_result();

    if (mc_dbg_433_disp_clear_force != 0U) {
        const uint32_t f = mc_dbg_433_disp_clear_force;
        mc_dbg_433_disp_clear_force = 0U;
        if (f == 1U) {
            (void)mc_dispset_clear_start();
        }
    }

    // ذخیره معلق: بعد از مکث یا درخواست فوری. اگر ذخیره الان ممکن نبود، دور بعد دوباره تلاش می‌شود.
    if (g_dirty && mc_time_elapsed_ms(g_dirty_ms, MC_UI_DISP_SAVE_DELAY_MS)) {
        if (mc_dispset_save_start()) {
            g_dirty = false;
        }
    }
}

}  // namespace mc
