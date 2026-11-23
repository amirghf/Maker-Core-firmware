// لیست منو و پیمایش با انکودر (بند ۸.۷ - بازطراحی ظاهر و اسکرول یک‌جا).
// هیچ هدر ST اینجا include نمی‌شود. فقط از hal و gfx و encoder استفاده می‌کند.
#include "ui_menu.h"
#include "ui_colors.h"
#include "ui_icons.h"
#include "../core/gfx.h"
#include "../core/text_format.h"
#include "../core/encoder.h"
#include "../hal/hal_display.h"

extern "C" {
volatile uint32_t mc_dbg_324_menu_sel_shown   = 0;  // خواندنی: اندیس آیتم هایلایت‌شده منو (۰ تا ۷)
volatile uint32_t mc_dbg_325_menu_first_shown = 0;  // خواندنی: اندیس اولین ردیف قابل مشاهده (۰ تا ۳)
volatile uint32_t mc_dbg_326_menu_enter_cnt   = 0;  // خواندنی: تعداد اینترهای منو روی آیتم‌های غیر BACK
volatile uint32_t mc_dbg_327_menu_enter_idx   = 255; // خواندنی: اندیس آیتم آخرین اینتر (۲۵۵ = هنوز نشده)
volatile uint32_t mc_dbg_328_menu_force       = 0;  // نوشتنی: ۰ = بی‌اثر؛ ۱ = ورود به منو؛ ۲ = برگشت به صفحه اصلی؛ ۳ تا ۱۰ = انتخاب آیتم
}

namespace mc {
namespace {

struct MenuItemSpec {
    const char* label;
    uint16_t    accent_color;
};

// پس‌زمینه تیره ردیف انتخاب‌شده (بند ۸.۷)
constexpr uint16_t kSelBg = mc_rgb565(24, 38, 64);

// لیست ۸ آیتم منو با ترتیب و اندیس دقیق (بند ۸.۷: آیتم اندیس ۲ برابر HOT BED است)
static const MenuItemSpec kMenuItems[8] = {
    { "MOVE",     mc_rgb565(80, 220, 120) },    // 0: MOVE
    { "EXTRUDER", kAccNozzle },                // 1: EXTRUDER
    { "HOT BED",  kAccBed },                   // 2: HOT BED (بند ۸.۷)
    { "SENSORS",  mc_rgb565(170, 120, 255) },  // 3: SENSORS
    { "SD CARD",  mc_rgb565(0, 200, 100) },    // 4: SD CARD
    { "USB OTG",  kYellowBusy },               // 5: USB OTG
    { "ABOUT",    kAccFan },                   // 6: ABOUT
    { "BACK",     kLabelGray }                 // 7: BACK
};

static uint8_t g_menu_sel   = 0U;   // اندیس آیتم انتخاب‌شده (۰ تا ۷)
static uint8_t g_menu_first = 0U;   // اندیس اولین ردیف دیده‌شده (۰ تا ۳)
static uint8_t g_menu_step  = 0U;   // گام نقاشی ورود به منو (۰ تا ۸، ۲۵۵ = کار عادی)

// رسم یک ردیف منو (row_idx = ۰ تا ۴)
static void draw_menu_row(uint8_t row_idx) {
    const uint8_t item_idx = static_cast<uint8_t>(g_menu_first + row_idx);
    if (item_idx >= 8U) { return; }

    const uint16_t ry = static_cast<uint16_t>(46U + static_cast<uint16_t>(row_idx) * 50U);
    const MenuItemSpec& spec = kMenuItems[item_idx];
    const bool is_sel = (item_idx == g_menu_sel);

    // ۱. پس‌زمینه ردیف
    const uint16_t bg = is_sel ? kSelBg : kScreenBg;
    mc_gfx_fill_rect(8U, ry, 448U, 44U, bg);

    // ۲. نوار عمودی باریک ۴ پیکسلی سمت چپ برای ردیف انتخاب‌شده
    if (is_sel) {
        mc_gfx_fill_rect(8U, ry, 4U, 44U, kAccentLine);
    }

    // ۳. پاک‌سازی ناحیه خط جداکننده و رسم خط نازک زیر ردیف
    mc_gfx_fill_rect(8U, static_cast<uint16_t>(ry + 44U), 448U, 6U, kScreenBg);
    mc_gfx_hline(14U, static_cast<uint16_t>(ry + 47U), 436U, kDividerGray);

    // ۴. آیکون ۳۲ در ۳۲
    const uint16_t icon_color = is_sel ? spec.accent_color : kIconBody;
    mc_icon_menu_item(20U, static_cast<uint16_t>(ry + 6U), item_idx, icon_color, bg);

    // ۵. نام آیتم با scale 2
    const uint16_t text_color = is_sel ? kColorWhite : kLabelGray;
    mc_gfx_text(64U, static_cast<uint16_t>(ry + 15U), spec.label, text_color, bg, 2U);

    // ۶. فلش نازک سمت راست با ضخامت ۲ پیکسل برای همه آیتم‌ها به جز BACK (اندیس ۷)
    if (item_idx != 7U) {
        const uint16_t arrow_color = is_sel ? kAccentLine : kDividerGray;
        for (uint16_t i = 0U; i <= 5U; ++i) {
            const uint16_t px = static_cast<uint16_t>(436U + i);
            const uint16_t y_upper = static_cast<uint16_t>(ry + 16U + i);
            const uint16_t y_lower = static_cast<uint16_t>(ry + 28U - i);

            mc_gfx_pixel(px, y_upper, arrow_color);
            mc_gfx_pixel(static_cast<uint16_t>(px + 1U), y_upper, arrow_color);

            mc_gfx_pixel(px, y_lower, arrow_color);
            mc_gfx_pixel(static_cast<uint16_t>(px + 1U), y_lower, arrow_color);
        }
    }
}

// رسم نوار اسکرول باریک (۳ پیکسل در x=470)
static void draw_scrollbar() {
    mc_gfx_fill_rect(464U, 46U, 13U, 244U, kScreenBg);  // پاک‌سازی نوار قدیمی
    mc_gfx_fill_rect(470U, 46U, 3U, 244U, kBarTrack);

    constexpr uint16_t thumb_h = (244U * 5U) / 8U;  // 152
    const uint16_t thumb_y = static_cast<uint16_t>(46U + (static_cast<uint16_t>(g_menu_first) * (244U - thumb_h)) / 3U);
    mc_gfx_fill_rect(470U, thumb_y, 3U, thumb_h, kAccentLine);
}

// اعمال تغییر انتخاب و به‌روزرسانی پنجره
static void apply_selection_change(uint8_t target_sel) {
    const uint8_t old_sel   = g_menu_sel;
    const uint8_t old_first = g_menu_first;
    g_menu_sel = target_sel;

    // محاسبه first جدید
    if (g_menu_sel < g_menu_first) {
        g_menu_first = g_menu_sel;
    } else if (g_menu_sel > g_menu_first + 4U) {
        g_menu_first = g_menu_sel - 4U;
    }

    mc_dbg_324_menu_sel_shown   = g_menu_sel;
    mc_dbg_325_menu_first_shown = g_menu_first;

    if (g_menu_first == old_first) {
        // تغییر فقط در همان پنجره: بازنقاشی فوری ۲ ردیف قبلی و جدید
        const uint8_t old_k = old_sel - old_first;
        const uint8_t new_k = g_menu_sel - g_menu_first;
        draw_menu_row(old_k);
        draw_menu_row(new_k);
    } else {
        // تغییر پنجره: بازنقاشی فوری تمام ۵ ردیف و نوار اسکرول
        for (uint8_t k = 0U; k < 5U; ++k) {
            draw_menu_row(k);
        }
        draw_scrollbar();
    }
}

}  // namespace

void mc_ui_menu_open_at(uint8_t item_idx) {
    g_menu_sel = (item_idx < 8U) ? item_idx : 0U;
    g_menu_first = 0U;
    if (g_menu_sel > 4U) {
        g_menu_first = static_cast<uint8_t>(g_menu_sel - 4U);
    }
    g_menu_step = 0U;   // شروع نقاشی ورود

    mc_dbg_324_menu_sel_shown   = g_menu_sel;
    mc_dbg_325_menu_first_shown = g_menu_first;

    (void)mc_encoder_take_clicks();
    (void)mc_encoder_take_press();
}

void mc_ui_menu_open() {
    mc_ui_menu_open_at(0U);
}

MenuResult mc_ui_menu_poll() {
    (void)mc_dbg_324_menu_sel_shown;
    (void)mc_dbg_325_menu_first_shown;
    (void)mc_dbg_326_menu_enter_cnt;
    (void)mc_dbg_327_menu_enter_idx;

    // ۱. بررسی انتخاب اجباری با متغیر دیباگ mc_dbg_328_menu_force (۰ تا ۱۰)
    if (mc_dbg_328_menu_force != 0U) {
        const uint32_t force_val = mc_dbg_328_menu_force;
        mc_dbg_328_menu_force = 0U;

        if (force_val == 2U) {
            // برگشت اجباری به صفحه اصلی
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return MenuResult::Back;
        } else if ((force_val >= 3U) && (force_val <= 10U)) {
            // انتخاب اجباری آیتم (مقدار - ۳)
            const uint8_t target_sel = static_cast<uint8_t>(force_val - 3U);
            if (target_sel != g_menu_sel) {
                apply_selection_change(target_sel);
            }
        }
    }

    // ۲. مراحل نقاشی اولیه‌ی ورود به منو (گام‌های ۰ تا ۸)
    if (g_menu_step < 255U) {
        switch (g_menu_step) {
            case 0: // پس‌زمینه اصلی
                mc_display_fill_screen(kScreenBg);
                g_menu_step = 1U;
                return MenuResult::Stay;

            case 1: { // نوار بالا + عنوان MENU
                mc_gfx_fill_rect(0U, 0U, 480U, 36U, kTopBarBg);
                mc_gfx_fill_rect(0U, 36U, 480U, 2U, kAccentLine);
                const uint16_t tw = mc_gfx_text_width("MENU", 2U);
                const uint16_t tx = static_cast<uint16_t>((480U - tw) / 2U);
                mc_gfx_text(tx, 10U, "MENU", kColorWhite, kTopBarBg, 2U);
                g_menu_step = 2U;
                return MenuResult::Stay;
            }

            case 2:
            case 3:
            case 4:
            case 5:
            case 6: { // نقاشی ردیف k (۰ تا ۴)
                const uint8_t k = static_cast<uint8_t>(g_menu_step - 2U);
                draw_menu_row(k);
                g_menu_step++;
                return MenuResult::Stay;
            }

            case 7: // نوار اسکرول
                draw_scrollbar();
                g_menu_step = 8U;
                return MenuResult::Stay;

            case 8: // اتمام نقاشی اولیه
                g_menu_step = 255U;
                return MenuResult::Stay;

            default:
                g_menu_step = 255U;
                return MenuResult::Stay;
        }
    }

    // ۳. پردازش کلیک‌های چرخش انکودر
    const int32_t clicks = mc_encoder_take_clicks();
    if (clicks != 0) {
        int32_t new_idx = static_cast<int32_t>(g_menu_sel) + clicks;
        while (new_idx < 0) {
            new_idx += 8;
        }
        while (new_idx >= 8) {
            new_idx %= 8;
        }
        const uint8_t target_sel = static_cast<uint8_t>(new_idx);
        if (target_sel != g_menu_sel) {
            apply_selection_change(target_sel);
        }
    }

    // ۴. پردازش دکمه انکودر (اینتر)
    if (mc_encoder_take_press()) {
        if (g_menu_sel == 7U) { // BACK
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return MenuResult::Back;
        } else if (g_menu_sel == 0U) { // MOVE (بند ۸.۸)
            mc_dbg_326_menu_enter_cnt = mc_dbg_326_menu_enter_cnt + 1U;
            mc_dbg_327_menu_enter_idx = 0U;
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return MenuResult::OpenMove;
        } else if (g_menu_sel == 1U) { // EXTRUDER (بند ۸.۹)
            mc_dbg_326_menu_enter_cnt = mc_dbg_326_menu_enter_cnt + 1U;
            mc_dbg_327_menu_enter_idx = 1U;
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return MenuResult::OpenExtruder;
        } else if (g_menu_sel == 2U) { // HOT BED (بند ۸.۱۰)
            mc_dbg_326_menu_enter_cnt = mc_dbg_326_menu_enter_cnt + 1U;
            mc_dbg_327_menu_enter_idx = 2U;
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return MenuResult::OpenBed;
        } else if (g_menu_sel == 3U) { // SENSORS (بند ۸.۱۲)
            mc_dbg_326_menu_enter_cnt = mc_dbg_326_menu_enter_cnt + 1U;
            mc_dbg_327_menu_enter_idx = 3U;
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return MenuResult::OpenSensors;
        } else if (g_menu_sel == 4U) { // SD CARD (بند ۸.۱۴)
            mc_dbg_326_menu_enter_cnt = mc_dbg_326_menu_enter_cnt + 1U;
            mc_dbg_327_menu_enter_idx = 4U;
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return MenuResult::OpenSd;
        } else if (g_menu_sel == 5U) { // USB OTG (بند ۸.۱۵)
            mc_dbg_326_menu_enter_cnt = mc_dbg_326_menu_enter_cnt + 1U;
            mc_dbg_327_menu_enter_idx = 5U;
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            return MenuResult::OpenOtg;
        } else {
            mc_dbg_326_menu_enter_cnt = mc_dbg_326_menu_enter_cnt + 1U;
            mc_dbg_327_menu_enter_idx = g_menu_sel;
            // عملکرد هر آیتم در بندهای بعدی وصل می‌شود
        }
    }

    return MenuResult::Stay;
}

}  // namespace mc
