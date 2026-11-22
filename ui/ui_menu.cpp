// لیست منو و پیمایش با انکودر (بند ۸.۷).
// هیچ هدر ST اینجا include نمی‌شود. فقط از hal و gfx و encoder استفاده می‌کند.
#include "ui_menu.h"
#include "ui_colors.h"
#include "ui_icons.h"
#include "../core/gfx.h"
#include "../core/text_format.h"
#include "../core/encoder.h"
#include "../hal/hal_display.h"

extern "C" {
volatile uint32_t mc_dbg_324_menu_sel_shown  = 0;  // خواندنی: اندیس آیتم هایلایت‌شده منو (۰ تا ۷)
volatile uint32_t mc_dbg_325_menu_first_shown = 0;  // خواندنی: اندیس اولین ردیف قابل مشاهده (۰ تا ۳)
volatile uint32_t mc_dbg_326_menu_enter_cnt  = 0;  // خواندنی: تعداد اینترهای منو روی آیتم‌های غیر BACK
volatile uint32_t mc_dbg_327_menu_enter_idx  = 255;  // خواندنی: اندیس آیتم آخرین اینتر (۲۵۵ = هنوز نشده)
volatile uint32_t mc_dbg_328_menu_force      = 0;  // نوشتنی: ۰ = بی‌اثر؛ ۱ = ورود به منو؛ ۲ = برگشت به صفحه اصلی؛ ۳ تا ۱۰ = انتخاب آیتم
}

namespace mc {
namespace {

struct MenuItemSpec {
    const char* label;
    uint16_t    accent_color;
};

// لیست ۸ آیتم منو با ترتیب و اندیس دقیق
static const MenuItemSpec kMenuItems[8] = {
    { "MOVE",     mc_rgb565(80, 220, 120) },    // 0: MOVE
    { "EXTRUDER", kAccNozzle },                // 1: EXTRUDER
    { "HEAT",     kAccBed },                   // 2: HEAT
    { "SENSORS",  mc_rgb565(170, 120, 255) },  // 3: SENSORS
    { "SD CARD",  mc_rgb565(0, 200, 100) },    // 4: SD CARD
    { "USB OTG",  kYellowBusy },               // 5: USB OTG
    { "ABOUT",    kAccFan },                   // 6: ABOUT
    { "BACK",     kLabelGray }                 // 7: BACK
};

static uint8_t g_menu_sel   = 0U;   // اندیس آیتم انتخاب‌شده (۰ تا ۷)
static uint8_t g_menu_first = 0U;   // اندیس اولین ردیف دیده‌شده (۰ تا ۳)
static uint8_t g_menu_step  = 0U;   // گام نقاشی (۰ تا ۸ ورود، ۱۰ تا ۱۶ بازنقاشی، ۲۵۵ کار عادی)

// رسم نوار بالا: شماره انتخاب‌شده با فرم 1/8 تا 8/8
static void draw_top_num() {
    char buf[8];
    uint16_t pos = 0U;
    pos = mc_fmt_uint(buf, sizeof(buf), pos, static_cast<uint32_t>(g_menu_sel + 1U));
    pos = mc_fmt_text(buf, sizeof(buf), pos, "/8");

    // پاک کردن پس‌زمینه ۵ کاراکتری (۳۰ در ۸ پیکسل) جهت جلوگیری از باقی‌ماندن متون قبلی
    mc_gfx_fill_rect(424U, 14U, 30U, 8U, kTopBarBg);
    mc_gfx_text(424U, 14U, buf, kLabelGray, kTopBarBg, 1U);
}

// رسم قاب هایلایت بیرون ردیف row_idx (۰ تا ۴)
static void draw_row_highlight(uint8_t row_idx, uint16_t color) {
    const uint16_t ry = static_cast<uint16_t>(46U + static_cast<uint16_t>(row_idx) * 50U);
    mc_gfx_fill_rect(6U, static_cast<uint16_t>(ry - 2U), 452U, 2U, color);     // بالا
    mc_gfx_fill_rect(6U, static_cast<uint16_t>(ry + 44U), 452U, 2U, color);    // پایین
    mc_gfx_fill_rect(6U, ry, 2U, 44U, color);                                   // چپ
    mc_gfx_fill_rect(456U, ry, 2U, 44U, color);                                 // راست
}

// رسم کارت و محتوای یک ردیف منو (row_idx = ۰ تا ۴)
static void draw_menu_row(uint8_t row_idx) {
    const uint8_t item_idx = g_menu_first + row_idx;
    if (item_idx >= 8U) { return; }

    const uint16_t ry = static_cast<uint16_t>(46U + static_cast<uint16_t>(row_idx) * 50U);
    const MenuItemSpec& spec = kMenuItems[item_idx];

    // پاک کردن ناحیه کارت و هایلایت قبلی با kScreenBg
    mc_gfx_fill_rect(6U, static_cast<uint16_t>(ry - 2U), 452U, 48U, kScreenBg);

    // کارت ردیف
    mc_ui_card(8U, ry, 448U, 44U, 8U, kCardBg, kCardBorder);

    // نوار باریک رنگی شاخص
    mc_gfx_fill_rect(8U, static_cast<uint16_t>(ry + 6U), 4U, 32U, spec.accent_color);

    // آیکون ۳۲ در ۳۲
    mc_icon_menu_item(20U, static_cast<uint16_t>(ry + 6U), item_idx, spec.accent_color, kCardBg);

    // نام آیتم با scale 2 سفید
    mc_gfx_text(64U, static_cast<uint16_t>(ry + 15U), spec.label, kColorWhite, kCardBg, 2U);

    // برای همه آیتم‌ها به‌جز BACK: مثلث رو به راست
    if (item_idx != 7U) {
        mc_ui_fill_triangle(432, static_cast<int16_t>(ry + 16U),
                            440, static_cast<int16_t>(ry + 22U),
                            432, static_cast<int16_t>(ry + 28U), kLabelGray);
    }
}

// رسم نوار اسکرول و دستگیره
static void draw_scrollbar() {
    mc_gfx_fill_rect(464U, 46U, 6U, 244U, kBarTrack);
    constexpr uint16_t thumb_h = (244U * 5U) / 8U;  // 152
    const uint16_t thumb_y = static_cast<uint16_t>(46U + (static_cast<uint16_t>(g_menu_first) * (244U - thumb_h)) / 3U);
    mc_gfx_fill_rect(464U, thumb_y, 6U, thumb_h, kAccentLine);
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

    mc_dbg_324_menu_sel_shown  = g_menu_sel;
    mc_dbg_325_menu_first_shown = g_menu_first;

    if (g_menu_first == old_first) {
        // فقط انتخاب در همان پنجره جابه‌جا شده است
        const uint8_t old_k = old_sel - old_first;
        const uint8_t new_k = g_menu_sel - g_menu_first;
        draw_row_highlight(old_k, kScreenBg);
        draw_row_highlight(new_k, kColorWhite);
        draw_top_num();
    } else {
        // پنجره تغییر کرده؛ شروع بازنقاشی مرحله‌ای ردیف‌ها
        g_menu_step = 10U;
    }
}

}  // namespace

void mc_ui_menu_open() {
    g_menu_sel   = 0U;
    g_menu_first = 0U;
    g_menu_step  = 0U;   // شروع نقاشی مرحله‌ای

    mc_dbg_324_menu_sel_shown  = 0U;
    mc_dbg_325_menu_first_shown = 0U;

    (void)mc_encoder_take_clicks();
    (void)mc_encoder_take_press();
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

    // ۲. مراحل نقاشی اولیه‌ی ورود به منو یا بازنقاشی ردیف‌ها
    if (g_menu_step < 255U) {
        switch (g_menu_step) {
            case 0: // پس‌زمینه اصلی
                mc_display_fill_screen(kScreenBg);
                g_menu_step = 1U;
                return MenuResult::Stay;

            case 1: { // نوار بالا + عنوان + شماره
                mc_gfx_fill_rect(0U, 0U, 480U, 36U, kTopBarBg);
                mc_gfx_fill_rect(0U, 36U, 480U, 2U, kAccentLine);
                const uint16_t tw = mc_gfx_text_width("MENU", 2U);
                const uint16_t tx = static_cast<uint16_t>((480U - tw) / 2U);
                mc_gfx_text(tx, 10U, "MENU", kColorWhite, kTopBarBg, 2U);
                draw_top_num();
                g_menu_step = 2U;
                return MenuResult::Stay;
            }

            case 2:
            case 3:
            case 4:
            case 5:
            case 6: { // نقاشی ردیف k (۰ تا ۴)
                const uint8_t k = g_menu_step - 2U;
                draw_menu_row(k);
                g_menu_step++;
                return MenuResult::Stay;
            }

            case 7: // نوار اسکرول
                draw_scrollbar();
                g_menu_step = 8U;
                return MenuResult::Stay;

            case 8: // هایلایت
                draw_row_highlight(g_menu_sel - g_menu_first, kColorWhite);
                g_menu_step = 255U; // اتمام نقاشی اولیه
                return MenuResult::Stay;

            // --- مراحل بازنقاشی هنگام تغییر first ---
            case 10:
            case 11:
            case 12:
            case 13:
            case 14: { // بازنقاشی یک ردیف
                const uint8_t k = g_menu_step - 10U;
                draw_menu_row(k);
                g_menu_step++;
                return MenuResult::Stay;
            }

            case 15: // بازنقاشی نوار اسکرول و شماره بالا
                draw_scrollbar();
                draw_top_num();
                g_menu_step = 16U;
                return MenuResult::Stay;

            case 16: // بازنقاشی هایلایت
                draw_row_highlight(g_menu_sel - g_menu_first, kColorWhite);
                g_menu_step = 255U; // اتمام بازنقاشی
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
        } else {
            mc_dbg_326_menu_enter_cnt = mc_dbg_326_menu_enter_cnt + 1U;
            mc_dbg_327_menu_enter_idx = g_menu_sel;
            // عملکرد هر آیتم در بندهای ۸.۸ تا ۸.۱۵ وصل می‌شود
        }
    }

    return MenuResult::Stay;
}

}  // namespace mc
