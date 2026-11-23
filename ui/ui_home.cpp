// صفحه اصلی رابط کاربری (بند ۸.۲).
// هیچ هدر ST اینجا include نمی‌شود.
#include "ui_home.h"
#include "ui_colors.h"
#include "ui_icons.h"
#include "ui_menu.h"
#include "ui_move.h"
#include "ui_extruder.h"
#include "ui_bed.h"
#include "ui_sensors.h"
#include "ui_sd.h"
#include "ui_otg.h"
#include "ui_about.h"
#include "ui_tcal.h"
#include <string.h>
#include "../core/gfx.h"
#include "../core/text_format.h"
#include "../core/thermistor.h"
#include "../core/heater_ex.h"
#include "../core/heater_bed.h"
#include "../core/machine_state.h"
#include "../core/output_pwm.h"
#include "../core/error.h"
#include "../core/log.h"
#include "../core/touch_input.h"
#include "../core/touch_cal_store.h"
#include "../core/settings_boot.h"
#include "../config/mc_version.h"
#include "../hal/hal_stepper.h"
#include "../hal/hal_display.h"
#include "../hal/hal_time.h"
#include "../core/encoder.h"


// متغیرهای دیباگ (CubeIDE Live Expressions)
extern "C" {
volatile uint32_t mc_dbg_293_ui_request         = 0;  // نوشتنی. اگر ۱ شود صفحه اصلی دوباره نقاشی می‌شود (بدون مکث) و خودش ۰ می‌شود
volatile uint32_t mc_dbg_294_ui_state           = 0;  // خواندنی. وضعیت UI (0 شروع‌نشده، 1 منتظر اسپلش، 2 مکث، 3 نقاشی قاب ثابت، 4 کار عادی، 5 غیرفعال)
volatile uint32_t mc_dbg_295_ui_draw_cnt        = 0;  // خواندنی. تعداد نقاشی‌های فیلد و مرحله قاب ثابت از روشن‌شدن
volatile uint32_t mc_dbg_296_ui_max_tick_us     = 0;  // خواندنی. بیشترین زمان یک بار اجرای mc_ui_poll (میکروثانیه)
volatile uint32_t mc_dbg_297_ui_fake_on         = 0;  // نوشتنی. ۱ = مقدارهای ساختگی دما، هدف و وضعیت به‌جای واقعی
volatile int32_t  mc_dbg_298_ui_fake_noz_x10     = 0;  // نوشتنی. دمای فعلی اکسترودر ×۱۰ (فقط وقتی fake روشن است). -9999 = سنسور خراب
volatile int32_t  mc_dbg_299_ui_fake_bed_x10     = 0;  // نوشتنی. دمای فعلی بستر ×۱۰. -9999 = سنسور خراب
volatile int32_t  mc_dbg_300_ui_fake_noz_tgt_x10 = 0;  // نوشتنی. هدف اکسترودر ×۱۰
volatile int32_t  mc_dbg_301_ui_fake_bed_tgt_x10 = 0;  // نوشتنی. هدف بستر ×۱۰
volatile uint32_t mc_dbg_302_ui_fake_status     = 0;  // نوشتنی. 0 = واقعی، 1 = READY، 2 = BUSY، 3 = KILLED، 4 = PRINTING، 5 = PAUSED (فقط ظاهر، با fake)
volatile uint32_t mc_dbg_303_ui_fake_media      = 0;  // نوشتنی. بیت 0 = SD، بیت 1 = OTG (فقط وقتی fake روشن است)
volatile uint32_t mc_dbg_304_ui_fake_fan_pct    = 0;  // نوشتنی. درصد فن 0 تا 100 (فقط وقتی fake روشن است)
volatile uint32_t mc_dbg_305_ui_fake_axis_en    = 0;  // نوشتنی. بیت 0 تا 3 = X, Y, Z, E (فقط وقتی fake روشن است)
volatile uint32_t mc_dbg_306_ui_media_shown     = 0;  // خواندنی. بیت 0 = SD موجود، بیت 1 = OTG موجود
volatile uint32_t mc_dbg_307_ui_fake_axis_dir   = 0;  // نوشتنی. بیت 0 تا 3 = جهت X, Y, Z, E (1 = مثبت، 0 = منفی). فقط وقتی fake روشن است
volatile uint32_t mc_dbg_308_ui_dir_shown        = 0;  // خواندنی. بیت 0 تا 3 = جهتی که الان برای X, Y, Z, E نشان داده می‌شود (1 = مثبت)
volatile uint32_t mc_dbg_311_ui_fake_src         = 0;  // نوشتنی. 0 = واقعی، 1 = USB، 2 = USB OTG، 3 = SD CARD. فقط وقتی fake روشن است
volatile uint32_t mc_dbg_312_ui_src_shown        = 0;  // خواندنی. کد منبعی که الان نشان داده می‌شود (1 تا 3)
volatile uint32_t mc_dbg_318_ui_axis_white_shown = 0;  // خواندنی. بیت 0 تا 3 = سلول X, Y, Z, E الان سفید (فعال) نشان داده می‌شود
volatile uint32_t mc_dbg_319_ui_sel_shown        = 0;  // خواندنی. اندیس گزینه‌ای که الان هایلایت دارد (0 = START، 1 = PAUSE، 2 = STOP، 3 = RESET، 4 = منو)
volatile uint32_t mc_dbg_320_ui_enter_cnt        = 0;  // خواندنی. تعداد دفعات فشردن دکمه انکودر (اینتر) در صفحه اصلی
volatile uint32_t mc_dbg_321_ui_enter_idx        = 255; // خواندنی. اندیس گزینه در آخرین اینتر (255 = هنوز نشده)
volatile uint32_t mc_dbg_322_ui_sel_force        = 0;  // نوشتنی. 0 = بی‌اثر؛ 1 تا 5 = گزینه (مقدار - 1) را انتخاب کن و خودش 0 می‌شود
volatile uint32_t mc_dbg_323_ui_page             = 0;  // خواندنی. 0 = صفحه اصلی، 1 = منو، 2 = حرکت، 3 = اکسترودر، 4 = داغ، 5 = سنسورها، 6 = SD CARD، 7 = USB OTG، 8 = ABOUT، 9 = TOUCH CAL
volatile uint32_t mc_dbg_359_ui_fake_print_pct  = 0;  // نوشتنی. درصد ساختگی چاپ، 0 تا 100 (فقط وقتی fake روشن است) (بند ۸.۶)
volatile uint32_t mc_dbg_362_touch_cal_force   = 0;  // نوشتنی: ۰ بی‌اثر، ۱ شروع، ۲ لغو، ۳ پاک‌کردن (بند ۸.۱۷)
volatile uint32_t mc_dbg_370_touch_cal_src     = 0;  // خواندنی: ۰ کالیبره نیست، ۱ از EEPROM، ۲ تازه کالیبره (بند ۸.۱۷)
volatile uint32_t mc_dbg_371_touch_save_result = 0;  // خواندنی: ۰ هنوز، ۱ موفق، ۲ ناموفق (بند ۸.۱۷)
}

namespace mc {
bool g_enc_pressed_at_boot = false;

namespace {

constexpr uint16_t kScreenBg            = mc_rgb565(8, 10, 16);       // پس‌زمینه اصلی صفحه
constexpr uint16_t kTopBarBg            = mc_rgb565(16, 44, 104);     // نوار بالا
constexpr uint16_t kAccentLine          = mc_rgb565(0, 200, 230);     // خط زیر نوار بالا
constexpr uint16_t kCardBg              = mc_rgb565(22, 27, 38);      // پس‌زمینه کارت‌ها و دکمه‌ها
constexpr uint16_t kCardBorder          = mc_rgb565(48, 58, 80);      // حاشیه کارت‌ها
constexpr uint16_t kBarTrack            = mc_rgb565(44, 52, 70);      // شیار نوار پیشرفت
constexpr uint16_t kBtnBg               = mc_rgb565(10, 30, 70);      // پس‌زمینه دکمه منو
constexpr uint16_t kBtnBorder           = mc_rgb565(60, 110, 200);    // حاشیه دکمه منو
constexpr uint16_t kAccNozzle           = mc_rgb565(255, 120, 30);    // رنگ شاخص اکسترودر
constexpr uint16_t kAccFan              = mc_rgb565(0, 200, 230);     // رنگ شاخص فن
constexpr uint16_t kAccBed              = mc_rgb565(240, 70, 70);     // رنگ شاخص بستر
constexpr uint16_t kIconBody            = mc_rgb565(120, 135, 165);   // بدنه آیکون‌ها
constexpr uint16_t kFanBody             = mc_rgb565(46, 58, 82);      // بدنه قاب آیکون فن
constexpr uint16_t kFanDisk             = mc_rgb565(10, 18, 28);      // دیسک داخلی آیکون فن
constexpr uint16_t kAxisX               = mc_rgb565(255, 80, 80);     // رنگ محور X
constexpr uint16_t kAxisY               = mc_rgb565(80, 220, 120);    // رنگ محور Y
constexpr uint16_t kAxisZ               = mc_rgb565(90, 150, 255);    // رنگ محور Z
constexpr uint16_t kAxisE               = mc_rgb565(255, 200, 60);    // رنگ محور E

constexpr uint16_t kActiveBorder        = mc_rgb565(200, 205, 215);   // حاشیه سلول فعال
constexpr uint16_t kAxisXDark           = mc_rgb565(200, 30, 30);     // حرف X تیره روی پس‌زمینه سفید
constexpr uint16_t kAxisYDark           = mc_rgb565(20, 150, 70);     // حرف Y تیره روی پس‌زمینه سفید
constexpr uint16_t kAxisZDark           = mc_rgb565(30, 90, 210);     // حرف Z تیره روی پس‌زمینه سفید
constexpr uint16_t kAxisEDark           = mc_rgb565(190, 130, 0);     // حرف E تیره روی پس‌زمینه سفید
constexpr uint16_t kActiveStatus        = mc_rgb565(0, 120, 50);      // متن ON تیره روی پس‌زمینه سفید
constexpr uint16_t kAxisPosDark         = mc_rgb565(20, 24, 34);      // رنگ مقدار موقعیت محور وقتی سلول سفید است (محور فعال)
constexpr uint16_t kDirDimInact         = mc_rgb565(60, 70, 90);      // فلش کم‌رنگ روی پس‌زمینه تاریک
constexpr uint16_t kDirDimActive        = mc_rgb565(190, 196, 208);   // فلش کم‌رنگ روی پس‌زمینه سفید
constexpr uint16_t kDirPosActive        = mc_rgb565(0, 170, 60);      // فلش پررنگ جهت مثبت
constexpr uint16_t kDirNegActive        = mc_rgb565(240, 120, 0);     // فلش پررنگ جهت منفی

constexpr uint16_t kIconOff             = mc_rgb565(100, 120, 160);   // آیکون SD/OTG وقتی غیرفعال است
constexpr uint16_t kCoolBlue            = mc_rgb565(100, 180, 255);   // حالت COOL
constexpr uint16_t kCyanPrint           = mc_rgb565(0, 220, 220);     // حالت PRINTING
constexpr uint16_t kLabelGray           = mc_rgb565(180, 180, 180);
constexpr uint16_t kDividerGray         = mc_rgb565(80, 80, 80);
constexpr uint16_t kYellowBusy          = mc_rgb565(255, 220, 0);
constexpr uint16_t kOrangeTgt           = mc_rgb565(255, 140, 0);

struct OptionRect {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    uint16_t bg_color;
};

static const OptionRect kOptionRects[5] = {
    {   8U, 236U, 110U, 56U, kScreenBg },  // 0: START
    { 126U, 236U, 110U, 56U, kScreenBg },  // 1: PAUSE
    { 244U, 236U, 110U, 56U, kScreenBg },  // 2: STOP
    { 362U, 236U, 110U, 56U, kScreenBg },  // 3: RESET
    { 432U,   4U,  40U, 28U, kTopBarBg }   // 4: MENU
};

static void draw_option_highlight(uint8_t opt_idx, uint16_t color) {
    if (opt_idx >= 5U) { return; }
    const OptionRect& opt = kOptionRects[opt_idx];
    const uint16_t fx = static_cast<uint16_t>(opt.x - 2U);
    const uint16_t fy = static_cast<uint16_t>(opt.y - 2U);
    const uint16_t fw = static_cast<uint16_t>(opt.w + 4U);

    // بالای قاب (۲ پیکسل)
    mc_gfx_fill_rect(fx, fy, fw, 2U, color);
    // پایین قاب (۲ پیکسل)
    mc_gfx_fill_rect(fx, static_cast<uint16_t>(opt.y + opt.h), fw, 2U, color);
    // چپ قاب (۲ پیکسل)
    mc_gfx_fill_rect(fx, opt.y, 2U, opt.h, color);
    // راست قاب (۲ پیکسل)
    mc_gfx_fill_rect(static_cast<uint16_t>(opt.x + opt.w), opt.y, 2U, opt.h, color);
}

struct FieldSpec {
    uint16_t x;
    uint16_t y;
    uint8_t  width;   // تعداد کاراکتر
    uint8_t  scale;
    uint16_t bg_color;
};

constexpr uint8_t kFieldCount = 25U;

static const FieldSpec kFields[kFieldCount] = {
    {  10U,   6U,  0U, 0U, kTopBarBg },  //  0: SD Icon
    {  46U,   6U,  0U, 0U, kTopBarBg },  //  1: USB Icon
    {  54U,  68U,  5U, 2U, kCardBg },    //  2: Extruder Status
    {  16U,  90U,  5U, 2U, kCardBg },    //  3: Extruder Current Temp
    {  90U,  90U,  3U, 2U, kCardBg },    //  4: Extruder Target Temp
    {  16U, 110U,  0U, 0U, kCardBg },    //  5: Extruder Progress Bar
    { 211U,  68U,  5U, 2U, kCardBg },    //  6: Fan Status
    { 235U,  84U,  4U, 3U, kCardBg },    //  7: Fan Percent (راست‌چین)
    { 173U, 110U,  0U, 0U, kCardBg },    //  8: Fan Progress Bar
    { 368U,  68U,  5U, 2U, kCardBg },    //  9: Bed Status
    { 330U,  90U,  5U, 2U, kCardBg },    // 10: Bed Current Temp
    { 404U,  90U,  3U, 2U, kCardBg },    // 11: Bed Target Temp
    { 330U, 110U,  0U, 0U, kCardBg },    // 12: Bed Progress Bar
    {  40U, 140U,  3U, 2U, kCardBg },    // 13: Axis X Cell
    { 158U, 140U,  3U, 2U, kCardBg },    // 14: Axis Y Cell
    { 276U, 140U,  3U, 2U, kCardBg },    // 15: Axis Z Cell
    { 394U, 140U,  3U, 2U, kCardBg },    // 16: Axis E Cell
    {  16U, 166U,  8U, 2U, kCardBg },    // 17: Axis X Position
    { 134U, 166U,  8U, 2U, kCardBg },    // 18: Axis Y Position
    { 252U, 166U,  8U, 2U, kCardBg },    // 19: Axis Z Position
    { 370U, 166U,  8U, 2U, kCardBg },    // 20: Axis E Position
    { 126U, 204U, 10U, 2U, kCardBg },    // 21: Printer Status Text
    { 366U, 204U,  7U, 2U, kCardBg },    // 22: Input Source Text
    {   8U, 302U,  0U, 0U, kScreenBg },  // 23: Print Progress Bar
    { 416U, 299U,  4U, 2U, kScreenBg }   // 24: Print Progress Percent
};

struct FieldCache {
    char     text[16];
    uint16_t fg_color;
    uint8_t  state;
    bool     dirty;
};

static FieldCache g_field_cache[kFieldCount];
static uint32_t   g_splash_hold_start_ms = 0U;
static uint8_t    g_static_frame_step    = 0U;
static uint8_t    g_current_field_idx    = 0U;

// تشخیص واقعی SD در بند ۹.۱ وصل می‌شود
static bool media_sd_present() {
    return false;
}

// تشخیص واقعی OTG در بند ۹.۴ وصل می‌شود
static bool media_otg_present() {
    return false;
}

// فعلاً فرمان‌ها فقط از کابل USB (CDC) می‌آیند. تشخیص واقعی SD در بند ۹.۳ و OTG در بند ۹.۴ وصل می‌شود
static uint32_t input_source_real() {
    return 1U; // 1 = USB
}

// در بند ۱۱.۶ به درصد چاپ (M73) وصل می‌شود
static uint32_t print_pct_real() {
    return 0U;
}

// بررسی فعال بودن یک محور (با پشتیبانی از حالت fake)
static bool is_axis_enabled(uint8_t axis_idx) {
    const bool fake = (mc_dbg_297_ui_fake_on != 0U);
    if (fake) {
        return (mc_dbg_305_ui_fake_axis_en & (1U << axis_idx)) != 0U;
    }
    return mc_stepper_is_enabled(static_cast<StepAxis>(axis_idx));
}

// پر کردن متن تا عرض ثابت با فاصله (space)
void pad_and_copy(char* dst, uint16_t cap, const char* src, uint8_t target_width) {
    uint16_t pos = 0U;
    while ((src[pos] != '\0') && (pos < target_width) && (pos + 1U < cap)) {
        dst[pos] = src[pos];
        pos++;
    }
    while ((pos < target_width) && (pos + 1U < cap)) {
        dst[pos] = ' ';
        pos++;
    }
    dst[pos] = '\0';
}

// راست‌چین کردن متن در عرض target_width با فاصله (space)
void right_and_copy(char* dst, uint16_t cap, const char* src, uint8_t target_width) {
    uint16_t len = 0U;
    while ((src[len] != '\0') && (len < target_width)) {
        len++;
    }
    const uint8_t pad_len = (target_width > len) ? static_cast<uint8_t>(target_width - len) : 0U;

    uint16_t pos = 0U;
    for (uint8_t i = 0U; (i < pad_len) && (pos + 1U < cap); ++i) {
        dst[pos++] = ' ';
    }
    for (uint16_t i = 0U; (i < len) && (pos + 1U < cap); ++i) {
        dst[pos++] = src[i];
    }
    dst[pos] = '\0';
}

// تولید متن و رنگ فیلدهای متنی
void format_field(uint8_t idx, char* out_buf, uint16_t cap, uint16_t& out_fg) {
    char raw[16];
    raw[0] = '\0';

    const bool fake = (mc_dbg_297_ui_fake_on != 0U);
    const FieldSpec& spec = kFields[idx];

    switch (idx) {
        case 2: { // وضعیت اکسترودر
            int32_t cur_x10 = 0;
            int32_t tgt_x10 = 0;
            bool sensor_ok = false;
            if (fake) {
                cur_x10 = mc_dbg_298_ui_fake_noz_x10;
                tgt_x10 = mc_dbg_300_ui_fake_noz_tgt_x10;
                sensor_ok = (cur_x10 != -9999);
            } else {
                sensor_ok = mc_temp_get_x10(AdcChannel::TempExtruder, &cur_x10);
                tgt_x10 = mc_heater_ex_target_x10();
            }

            if (!sensor_ok) {
                mc_fmt_text(raw, sizeof(raw), 0U, "ERR");
                out_fg = kColorRed;
            } else if (tgt_x10 <= 0) {
                mc_fmt_text(raw, sizeof(raw), 0U, "OFF");
                out_fg = kLabelGray;
            } else if (cur_x10 < tgt_x10 - MC_UI_TEMP_NEAR_X10) {
                mc_fmt_text(raw, sizeof(raw), 0U, "HEAT");
                out_fg = kOrangeTgt;
            } else if (cur_x10 > tgt_x10 + MC_UI_TEMP_NEAR_X10) {
                mc_fmt_text(raw, sizeof(raw), 0U, "COOL");
                out_fg = kCoolBlue;
            } else {
                mc_fmt_text(raw, sizeof(raw), 0U, "READY");
                out_fg = kColorGreen;
            }
            pad_and_copy(out_buf, cap, raw, spec.width);
            break;
        }

        case 3: { // دمای فعلی اکسترودر
            if (fake) {
                if (mc_dbg_298_ui_fake_noz_x10 == -9999) {
                    mc_fmt_text(raw, sizeof(raw), 0U, "---");
                    out_fg = kColorRed;
                } else {
                    const float val = static_cast<float>(mc_dbg_298_ui_fake_noz_x10) / 10.0f;
                    mc_fmt_float(raw, sizeof(raw), 0U, val, 1U);
                    out_fg = kColorWhite;
                }
            } else {
                int32_t t_x10 = 0;
                if (mc_temp_get_x10(AdcChannel::TempExtruder, &t_x10)) {
                    const float val = static_cast<float>(t_x10) / 10.0f;
                    mc_fmt_float(raw, sizeof(raw), 0U, val, 1U);
                    out_fg = kColorWhite;
                } else {
                    mc_fmt_text(raw, sizeof(raw), 0U, "---");
                    out_fg = kColorRed;
                }
            }
            pad_and_copy(out_buf, cap, raw, spec.width);
            break;
        }

        case 4: { // هدف اکسترودر
            const int32_t tgt_x10 = fake ? mc_dbg_300_ui_fake_noz_tgt_x10 : mc_heater_ex_target_x10();
            if (tgt_x10 <= 0) {
                mc_fmt_text(raw, sizeof(raw), 0U, "0");
                out_fg = kLabelGray;
            } else {
                const uint32_t deg = static_cast<uint32_t>((tgt_x10 + 5) / 10);
                mc_fmt_uint(raw, sizeof(raw), 0U, deg);
                out_fg = kOrangeTgt;
            }
            pad_and_copy(out_buf, cap, raw, spec.width);
            break;
        }

        case 6: { // وضعیت فن
            uint32_t pct = 0U;
            if (fake) {
                pct = (mc_dbg_304_ui_fake_fan_pct > 100U) ? 100U : mc_dbg_304_ui_fake_fan_pct;
            } else {
                pct = static_cast<uint32_t>((mc_fan_get() * 100U + 127U) / 255U);
            }
            if (pct == 0U) {
                mc_fmt_text(raw, sizeof(raw), 0U, "OFF");
                out_fg = kLabelGray;
            } else {
                mc_fmt_text(raw, sizeof(raw), 0U, "ON");
                out_fg = kColorGreen;
            }
            pad_and_copy(out_buf, cap, raw, spec.width);
            break;
        }

        case 7: { // درصد فن (راست‌چین)
            uint32_t pct = 0U;
            if (fake) {
                pct = (mc_dbg_304_ui_fake_fan_pct > 100U) ? 100U : mc_dbg_304_ui_fake_fan_pct;
            } else {
                pct = static_cast<uint32_t>((mc_fan_get() * 100U + 127U) / 255U);
            }
            const uint16_t pos = mc_fmt_uint(raw, sizeof(raw), 0U, pct);
            if (pos < sizeof(raw) - 1U) {
                raw[pos] = '%';
                raw[pos + 1U] = '\0';
            }
            out_fg = (pct == 0U) ? kLabelGray : kColorWhite;
            right_and_copy(out_buf, cap, raw, spec.width);
            break;
        }

        case 9: { // وضعیت بستر
            int32_t cur_x10 = 0;
            int32_t tgt_x10 = 0;
            bool sensor_ok = false;
            if (fake) {
                cur_x10 = mc_dbg_299_ui_fake_bed_x10;
                tgt_x10 = mc_dbg_301_ui_fake_bed_tgt_x10;
                sensor_ok = (cur_x10 != -9999);
            } else {
                sensor_ok = mc_temp_get_x10(AdcChannel::TempBed, &cur_x10);
                tgt_x10 = mc_heater_bed_target_x10();
            }

            if (!sensor_ok) {
                mc_fmt_text(raw, sizeof(raw), 0U, "ERR");
                out_fg = kColorRed;
            } else if (tgt_x10 <= 0) {
                mc_fmt_text(raw, sizeof(raw), 0U, "OFF");
                out_fg = kLabelGray;
            } else if (cur_x10 < tgt_x10 - MC_UI_TEMP_NEAR_X10) {
                mc_fmt_text(raw, sizeof(raw), 0U, "HEAT");
                out_fg = kOrangeTgt;
            } else if (cur_x10 > tgt_x10 + MC_UI_TEMP_NEAR_X10) {
                mc_fmt_text(raw, sizeof(raw), 0U, "COOL");
                out_fg = kCoolBlue;
            } else {
                mc_fmt_text(raw, sizeof(raw), 0U, "READY");
                out_fg = kColorGreen;
            }
            pad_and_copy(out_buf, cap, raw, spec.width);
            break;
        }

        case 10: { // دمای فعلی بستر
            if (fake) {
                if (mc_dbg_299_ui_fake_bed_x10 == -9999) {
                    mc_fmt_text(raw, sizeof(raw), 0U, "---");
                    out_fg = kColorRed;
                } else {
                    const float val = static_cast<float>(mc_dbg_299_ui_fake_bed_x10) / 10.0f;
                    mc_fmt_float(raw, sizeof(raw), 0U, val, 1U);
                    out_fg = kColorWhite;
                }
            } else {
                int32_t t_x10 = 0;
                if (mc_temp_get_x10(AdcChannel::TempBed, &t_x10)) {
                    const float val = static_cast<float>(t_x10) / 10.0f;
                    mc_fmt_float(raw, sizeof(raw), 0U, val, 1U);
                    out_fg = kColorWhite;
                } else {
                    mc_fmt_text(raw, sizeof(raw), 0U, "---");
                    out_fg = kColorRed;
                }
            }
            pad_and_copy(out_buf, cap, raw, spec.width);
            break;
        }

        case 11: { // هدف بستر
            const int32_t tgt_x10 = fake ? mc_dbg_301_ui_fake_bed_tgt_x10 : mc_heater_bed_target_x10();
            if (tgt_x10 <= 0) {
                mc_fmt_text(raw, sizeof(raw), 0U, "0");
                out_fg = kLabelGray;
            } else {
                const uint32_t deg = static_cast<uint32_t>((tgt_x10 + 5) / 10);
                mc_fmt_uint(raw, sizeof(raw), 0U, deg);
                out_fg = kOrangeTgt;
            }
            pad_and_copy(out_buf, cap, raw, spec.width);
            break;
        }

        case 17:
        case 18:
        case 19:
        case 20: { // موقعیت محورها X، Y، Z، E
            const uint8_t axis_code = idx - 17U;
            const float pos = mc_position_get(axis_code);
            const bool enabled = is_axis_enabled(axis_code);

            mc_fmt_float(raw, sizeof(raw), 0U, pos, 2U);
            out_fg = enabled ? kAxisPosDark : kColorWhite;
            pad_and_copy(out_buf, cap, raw, spec.width);
            break;
        }

        case 21: { // وضعیت پرینتر
            uint32_t st_code = 0U;
            if (fake && (mc_dbg_302_ui_fake_status != 0U)) {
                st_code = mc_dbg_302_ui_fake_status;
            } else {
                if (mc_is_killed()) {
                    st_code = 3U; // KILLED
                } else if (mc_motion_busy()) {
                    st_code = 2U; // BUSY
                } else {
                    st_code = 1U; // READY
                }
            }

            if (st_code == 3U) {
                mc_fmt_text(raw, sizeof(raw), 0U, "KILLED");
                out_fg = kColorRed;
            } else if (st_code == 2U) {
                mc_fmt_text(raw, sizeof(raw), 0U, "BUSY");
                out_fg = kYellowBusy;
            } else if (st_code == 4U) {
                mc_fmt_text(raw, sizeof(raw), 0U, "PRINTING");
                out_fg = kCyanPrint;
            } else if (st_code == 5U) {
                mc_fmt_text(raw, sizeof(raw), 0U, "PAUSED");
                out_fg = kOrangeTgt;
            } else {
                mc_fmt_text(raw, sizeof(raw), 0U, "READY");
                out_fg = kColorGreen;
            }
            pad_and_copy(out_buf, cap, raw, spec.width);
            break;
        }

        case 22: { // منبع ورودی (USB / USB OTG / SD CARD)
            uint32_t src_code = 1U;
            if (fake && (mc_dbg_311_ui_fake_src != 0U)) {
                src_code = mc_dbg_311_ui_fake_src;
            } else {
                src_code = input_source_real();
            }

            if (src_code == 2U) {
                mc_fmt_text(raw, sizeof(raw), 0U, "USB OTG");
                out_fg = kYellowBusy;
            } else if (src_code == 3U) {
                mc_fmt_text(raw, sizeof(raw), 0U, "SD CARD");
                out_fg = kColorGreen;
            } else {
                src_code = 1U;
                mc_fmt_text(raw, sizeof(raw), 0U, "USB");
                out_fg = kAccFan;
            }
            mc_dbg_312_ui_src_shown = src_code;
            pad_and_copy(out_buf, cap, raw, spec.width);
            break;
        }

        case 24: { // درصد پیشرفت چاپ (راست‌چین)
            uint32_t pct = 0U;
            if (fake) {
                pct = (mc_dbg_359_ui_fake_print_pct > 100U) ? 100U : mc_dbg_359_ui_fake_print_pct;
            } else {
                pct = print_pct_real();
            }
            const uint16_t pos = mc_fmt_uint(raw, sizeof(raw), 0U, pct);
            if (pos < sizeof(raw) - 1U) {
                raw[pos] = '%';
                raw[pos + 1U] = '\0';
            }
            out_fg = kColorWhite;
            right_and_copy(out_buf, cap, raw, spec.width);
            break;
        }

        default:
            break;
    }
}

// نقاشی یک مرحله از قاب ثابت (۱۷ مرحله، ۰ تا ۱۶)
void draw_static_step(uint8_t step) {
    switch (step) {
        case 0:
            mc_display_fill_screen(kScreenBg);
            break;
        case 1:
            mc_gfx_fill_rect(0U, 0U, 480U, 36U, kTopBarBg);
            mc_gfx_fill_rect(0U, 36U, 480U, 2U, kAccentLine);
            break;
        case 2:
            mc_ui_card(432U, 4U, 40U, 28U, 6U, kBtnBg, kBtnBorder);
            mc_icon_menu(440U, 6U, kColorWhite);
            break;
        case 3: {
            char dev_name[25];
            mc_fmt_text(dev_name, sizeof(dev_name), 0U, MC_DEVICE_NAME);
            const uint16_t tw = mc_gfx_text_width(dev_name, 2U);
            const uint16_t nx = (tw < 480U) ? static_cast<uint16_t>((480U - tw) / 2U) : 0U;
            mc_gfx_text(nx, 10U, dev_name, kColorWhite, kTopBarBg, 2U);
            break;
        }
        case 4: // کارت اکسترودر
            mc_ui_card(8U, 44U, 150U, 80U, 8U, kCardBg, kCardBorder);
            mc_gfx_fill_rect(18U, 45U, 130U, 2U, kAccNozzle);
            mc_icon_nozzle(16U, 52U, kAccNozzle, kIconBody);
            mc_gfx_text(54U, 54U, "NOZZLE", kLabelGray, kCardBg, 1U);
            mc_gfx_text(78U, 90U, "/", kColorWhite, kCardBg, 2U);
            mc_gfx_text(130U, 90U, "C", kColorWhite, kCardBg, 2U);
            mc_gfx_fill_rect(16U, 110U, 134U, 6U, kBarTrack);
            break;
        case 5: // کارت فن
            mc_ui_card(165U, 44U, 150U, 80U, 8U, kCardBg, kCardBorder);
            mc_gfx_fill_rect(175U, 45U, 130U, 2U, kAccFan);
            mc_icon_fan(173U, 52U, kAccFan, kFanBody, kFanDisk);
            mc_gfx_text(211U, 54U, "FAN", kLabelGray, kCardBg, 1U);
            mc_gfx_fill_rect(173U, 110U, 134U, 6U, kBarTrack);
            break;
        case 6: // کارت بستر
            mc_ui_card(322U, 44U, 150U, 80U, 8U, kCardBg, kCardBorder);
            mc_gfx_fill_rect(332U, 45U, 130U, 2U, kAccBed);
            mc_icon_bed(330U, 52U, kAccBed, kIconBody);
            mc_gfx_text(368U, 54U, "BED", kLabelGray, kCardBg, 1U);
            mc_gfx_text(392U, 90U, "/", kColorWhite, kCardBg, 2U);
            mc_gfx_text(444U, 90U, "C", kColorWhite, kCardBg, 2U);
            mc_gfx_fill_rect(330U, 110U, 134U, 6U, kBarTrack);
            break;
        case 7: // سلول محور X
            mc_ui_card(8U, 132U, 110U, 56U, 8U, kCardBg, kCardBorder);
            mc_gfx_text(16U, 138U, "X", kAxisX, kCardBg, 3U);
            break;
        case 8: // سلول محور Y
            mc_ui_card(126U, 132U, 110U, 56U, 8U, kCardBg, kCardBorder);
            mc_gfx_text(134U, 138U, "Y", kAxisY, kCardBg, 3U);
            break;
        case 9: // سلول محور Z
            mc_ui_card(244U, 132U, 110U, 56U, 8U, kCardBg, kCardBorder);
            mc_gfx_text(252U, 138U, "Z", kAxisZ, kCardBg, 3U);
            break;
        case 10: // سلول محور E
            mc_ui_card(362U, 132U, 110U, 56U, 8U, kCardBg, kCardBorder);
            mc_gfx_text(370U, 138U, "E", kAxisE, kCardBg, 3U);
            break;
        case 11: // نوار ادغام‌شده وضعیت دستگاه و منبع ورودی
            mc_ui_card(8U, 196U, 464U, 32U, 10U, kCardBg, kCardBorder);
            mc_gfx_text(30U, 204U, "STATUS:", kLabelGray, kCardBg, 2U);
            mc_gfx_text(270U, 204U, "SOURCE:", kLabelGray, kCardBg, 2U);
            break;
        case 12: { // دکمه START
            mc_ui_card(8U, 236U, 110U, 56U, 8U, kCardBg, kCardBorder);
            mc_icon_play(47U, 242U, kColorGreen);
            const uint16_t tw = mc_gfx_text_width("START", 1U);
            const uint16_t tx = static_cast<uint16_t>(8U + (110U - tw) / 2U);
            mc_gfx_text(tx, 280U, "START", kLabelGray, kCardBg, 1U);
            break;
        }
        case 13: { // دکمه PAUSE
            mc_ui_card(126U, 236U, 110U, 56U, 8U, kCardBg, kCardBorder);
            mc_icon_pause(165U, 242U, kYellowBusy);
            const uint16_t tw = mc_gfx_text_width("PAUSE", 1U);
            const uint16_t tx = static_cast<uint16_t>(126U + (110U - tw) / 2U);
            mc_gfx_text(tx, 280U, "PAUSE", kLabelGray, kCardBg, 1U);
            break;
        }
        case 14: { // دکمه STOP
            mc_ui_card(244U, 236U, 110U, 56U, 8U, kCardBg, kCardBorder);
            mc_icon_stop(283U, 242U, kColorRed);
            const uint16_t tw = mc_gfx_text_width("STOP", 1U);
            const uint16_t tx = static_cast<uint16_t>(244U + (110U - tw) / 2U);
            mc_gfx_text(tx, 280U, "STOP", kLabelGray, kCardBg, 1U);
            break;
        }
        case 15: { // دکمه RESET
            mc_ui_card(362U, 236U, 110U, 56U, 8U, kCardBg, kCardBorder);
            mc_icon_reset(401U, 242U, kOrangeTgt);
            const uint16_t tw = mc_gfx_text_width("RESET", 1U);
            const uint16_t tx = static_cast<uint16_t>(362U + (110U - tw) / 2U);
            mc_gfx_text(tx, 280U, "RESET", kLabelGray, kCardBg, 1U);
            break;
        }
        default:
            break;
    }
    mc_dbg_295_ui_draw_cnt = mc_dbg_295_ui_draw_cnt + 1U;
}

// بررسی و نقاشی یک فیلد متغیر در صورت تغییر
void update_one_field() {
    const uint8_t idx = g_current_field_idx;
    const FieldSpec& spec = kFields[idx];
    FieldCache& cache = g_field_cache[idx];

    if (idx == 0U) { // آیکون SD
        const bool fake = (mc_dbg_297_ui_fake_on != 0U);
        const bool sd_on = fake ? ((mc_dbg_303_ui_fake_media & 0x01U) != 0U) : media_sd_present();
        const bool otg_on = fake ? ((mc_dbg_303_ui_fake_media & 0x02U) != 0U) : media_otg_present();
        mc_dbg_306_ui_media_shown = (sd_on ? 0x01U : 0U) | (otg_on ? 0x02U : 0U);

        const uint8_t state = sd_on ? 1U : 0U;
        const uint16_t color = sd_on ? kColorGreen : kIconOff;
        if (cache.dirty || (cache.state != state)) {
            mc_icon_sd(spec.x, spec.y, color, spec.bg_color);
            cache.state = state;
            cache.dirty = false;
            mc_dbg_295_ui_draw_cnt = mc_dbg_295_ui_draw_cnt + 1U;
        }
    } else if (idx == 1U) { // آیکون USB
        const bool fake = (mc_dbg_297_ui_fake_on != 0U);
        const bool sd_on = fake ? ((mc_dbg_303_ui_fake_media & 0x01U) != 0U) : media_sd_present();
        const bool otg_on = fake ? ((mc_dbg_303_ui_fake_media & 0x02U) != 0U) : media_otg_present();
        mc_dbg_306_ui_media_shown = (sd_on ? 0x01U : 0U) | (otg_on ? 0x02U : 0U);

        const uint8_t state = otg_on ? 1U : 0U;
        const uint16_t color = otg_on ? kColorGreen : kIconOff;
        if (cache.dirty || (cache.state != state)) {
            mc_icon_usb(spec.x, spec.y, color, spec.bg_color);
            cache.state = state;
            cache.dirty = false;
            mc_dbg_295_ui_draw_cnt = mc_dbg_295_ui_draw_cnt + 1U;
        }
    } else if (idx == 5U) { // نوار پیشرفت اکسترودر
        const bool fake = (mc_dbg_297_ui_fake_on != 0U);
        int32_t cur_x10 = 0;
        int32_t tgt_x10 = 0;
        bool sensor_ok = false;
        if (fake) {
            cur_x10 = mc_dbg_298_ui_fake_noz_x10;
            tgt_x10 = mc_dbg_300_ui_fake_noz_tgt_x10;
            sensor_ok = (cur_x10 != -9999);
        } else {
            sensor_ok = mc_temp_get_x10(AdcChannel::TempExtruder, &cur_x10);
            tgt_x10 = mc_heater_ex_target_x10();
        }

        uint16_t fill_px = 0U;
        uint16_t fill_color = kAccNozzle;

        if (sensor_ok && (tgt_x10 > 0) && (cur_x10 > 0)) {
            int32_t px = (cur_x10 * 134) / tgt_x10;
            if (px < 0) { px = 0; }
            if (px > 134) { px = 134; }
            fill_px = static_cast<uint16_t>(px);

            if (cur_x10 < tgt_x10 - MC_UI_TEMP_NEAR_X10) {
                fill_color = kAccNozzle;
            } else if (cur_x10 > tgt_x10 + MC_UI_TEMP_NEAR_X10) {
                fill_color = kCoolBlue;
            } else {
                fill_color = kColorGreen;
            }
        }

        const uint8_t state = static_cast<uint8_t>(fill_px);
        if (cache.dirty || (cache.state != state) || (cache.fg_color != fill_color)) {
            mc_ui_progress(spec.x, spec.y, 134U, 6U, fill_px, fill_color, kBarTrack);
            cache.state = state;
            cache.fg_color = fill_color;
            cache.dirty = false;
            mc_dbg_295_ui_draw_cnt = mc_dbg_295_ui_draw_cnt + 1U;
        }
    } else if (idx == 8U) { // نوار پیشرفت فن
        const bool fake = (mc_dbg_297_ui_fake_on != 0U);
        uint32_t pct = 0U;
        if (fake) {
            pct = (mc_dbg_304_ui_fake_fan_pct > 100U) ? 100U : mc_dbg_304_ui_fake_fan_pct;
        } else {
            pct = static_cast<uint32_t>((mc_fan_get() * 100U + 127U) / 255U);
        }

        uint16_t fill_px = static_cast<uint16_t>((pct * 134U) / 100U);
        if (fill_px > 134U) { fill_px = 134U; }

        const uint8_t state = static_cast<uint8_t>(fill_px);
        if (cache.dirty || (cache.state != state)) {
            mc_ui_progress(spec.x, spec.y, 134U, 6U, fill_px, kAccFan, kBarTrack);
            cache.state = state;
            cache.fg_color = kAccFan;
            cache.dirty = false;
            mc_dbg_295_ui_draw_cnt = mc_dbg_295_ui_draw_cnt + 1U;
        }
    } else if (idx == 12U) { // نوار پیشرفت بستر
        const bool fake = (mc_dbg_297_ui_fake_on != 0U);
        int32_t cur_x10 = 0;
        int32_t tgt_x10 = 0;
        bool sensor_ok = false;
        if (fake) {
            cur_x10 = mc_dbg_299_ui_fake_bed_x10;
            tgt_x10 = mc_dbg_301_ui_fake_bed_tgt_x10;
            sensor_ok = (cur_x10 != -9999);
        } else {
            sensor_ok = mc_temp_get_x10(AdcChannel::TempBed, &cur_x10);
            tgt_x10 = mc_heater_bed_target_x10();
        }

        uint16_t fill_px = 0U;
        uint16_t fill_color = kAccBed;

        if (sensor_ok && (tgt_x10 > 0) && (cur_x10 > 0)) {
            int32_t px = (cur_x10 * 134) / tgt_x10;
            if (px < 0) { px = 0; }
            if (px > 134) { px = 134; }
            fill_px = static_cast<uint16_t>(px);

            if (cur_x10 < tgt_x10 - MC_UI_TEMP_NEAR_X10) {
                fill_color = kAccBed;
            } else if (cur_x10 > tgt_x10 + MC_UI_TEMP_NEAR_X10) {
                fill_color = kCoolBlue;
            } else {
                fill_color = kColorGreen;
            }
        }

        const uint8_t state = static_cast<uint8_t>(fill_px);
        if (cache.dirty || (cache.state != state) || (cache.fg_color != fill_color)) {
            mc_ui_progress(spec.x, spec.y, 134U, 6U, fill_px, fill_color, kBarTrack);
            cache.state = state;
            cache.fg_color = fill_color;
            cache.dirty = false;
            mc_dbg_295_ui_draw_cnt = mc_dbg_295_ui_draw_cnt + 1U;
        }
    } else if (idx == 23U) { // نوار پیشرفت پایین صفحه اصلی
        const bool fake = (mc_dbg_297_ui_fake_on != 0U);
        uint32_t pct = 0U;
        if (fake) {
            pct = (mc_dbg_359_ui_fake_print_pct > 100U) ? 100U : mc_dbg_359_ui_fake_print_pct;
        } else {
            pct = print_pct_real();
        }

        const uint16_t fill_px = static_cast<uint16_t>(pct * 4U);

        // ⚠️ نکته مهم: در FieldCache فیلد state از نوع uint8_t است و fill_px تا 400 در آن جا نمی‌شود.
        // برای نوار 23 مقدار state را برابر خود pct (0 تا 100) بگذار، نه fill_px.
        const uint8_t state = static_cast<uint8_t>(pct);
        if (cache.dirty || (cache.state != state)) {
            mc_ui_progress(spec.x, spec.y, 400U, 10U, fill_px, kCyanPrint, kBarTrack);
            cache.state = state;
            cache.fg_color = kCyanPrint;
            cache.dirty = false;
            mc_dbg_295_ui_draw_cnt = mc_dbg_295_ui_draw_cnt + 1U;
        }
    } else if (idx >= 13U && idx <= 16U) { // سلول محورهای X، Y، Z، E (کارت، برچسب، ON/OFF، فلش جفت‌مثلث)
        const uint8_t axis_idx = idx - 13U;
        const StepAxis axis = static_cast<StepAxis>(axis_idx);
        const bool fake = (mc_dbg_297_ui_fake_on != 0U);

        const bool enabled  = is_axis_enabled(axis_idx);
        const bool positive = fake ? ((mc_dbg_307_ui_fake_axis_dir & (1U << axis_idx)) != 0U)
                                   : (mc_stepper_get_dir(axis) == 1);

        // به‌روزرسانی متغیرهای دیباگ 308 و 318
        if (positive) {
            mc_dbg_308_ui_dir_shown |= (1U << axis_idx);
        } else {
            mc_dbg_308_ui_dir_shown &= ~(1U << axis_idx);
        }

        if (enabled) {
            mc_dbg_318_ui_axis_white_shown |= (1U << axis_idx);
        } else {
            mc_dbg_318_ui_axis_white_shown &= ~(1U << axis_idx);
        }

        const uint8_t state = (enabled ? 1U : 0U) | (positive ? 2U : 0U);
        if (cache.dirty || (cache.state != state)) {
            const uint16_t cell_x0 = static_cast<uint16_t>(8U + axis_idx * 118U);
            const uint16_t card_bg  = enabled ? kColorWhite : kCardBg;
            const uint16_t card_brd = enabled ? kActiveBorder : kCardBorder;

            static const uint16_t kInactLetters[4] = { kAxisX, kAxisY, kAxisZ, kAxisE };
            static const uint16_t kActLetters[4]   = { kAxisXDark, kAxisYDark, kAxisZDark, kAxisEDark };
            static const char* const kLetterStr[4] = { "X", "Y", "Z", "E" };

            const uint16_t letter_fg = enabled ? kActLetters[axis_idx] : kInactLetters[axis_idx];
            const char* const st_str  = enabled ? "ON" : "OFF";
            const uint16_t st_fg      = enabled ? kActiveStatus : kLabelGray;

            // ۱. کشیدن مجدد کارت، حرف محور و وضعیت ON/OFF
            mc_ui_card(cell_x0, 132U, 110U, 56U, 8U, card_bg, card_brd);
            mc_gfx_text(static_cast<uint16_t>(cell_x0 + 8U), 138U, kLetterStr[axis_idx], letter_fg, card_bg, 3U);
            mc_gfx_text(static_cast<uint16_t>(cell_x0 + 40U), 140U, st_str, st_fg, card_bg, 2U);

            // ۲. پاک‌کردن جعبه فلش و کشیدن فلش جفت‌مثلث بدون دم
            mc_gfx_fill_rect(static_cast<uint16_t>(cell_x0 + 82U), 137U, 18U, 26U, card_bg);
            const uint8_t act_code = enabled ? (positive ? 1U : 2U) : 0U;
            const uint16_t dim_col = enabled ? kDirDimActive : kDirDimInact;
            const uint16_t act_col = positive ? kDirPosActive : kDirNegActive;
            mc_icon_dir_pair(static_cast<uint16_t>(cell_x0 + 84U), 139U, act_code, act_col, dim_col);

            cache.state = state;
            cache.dirty = false;
            mc_dbg_295_ui_draw_cnt = mc_dbg_295_ui_draw_cnt + 1U;

            // ۳. dirty کردن فیلد موقعیت همین محور تا با پس‌زمینه جدید کشیده شود
            g_field_cache[17U + axis_idx].dirty = true;
        }
    } else { // سایر فیلدهای متنی
        char new_text[16];
        uint16_t new_fg = kColorWhite;
        format_field(idx, new_text, sizeof(new_text), new_fg);

        uint16_t bg_col = spec.bg_color;
        if (idx >= 17U && idx <= 20U) {
            const uint8_t axis_idx = idx - 17U;
            if (is_axis_enabled(axis_idx)) {
                bg_col = kColorWhite;
            }
        }

        if (cache.dirty || (cache.fg_color != new_fg) || (strcmp(cache.text, new_text) != 0)) {
            mc_gfx_text(spec.x, spec.y, new_text, new_fg, bg_col, spec.scale);
            for (uint16_t i = 0U; i < sizeof(cache.text); ++i) {
                cache.text[i] = new_text[i];
                if (new_text[i] == '\0') { break; }
            }
            cache.fg_color = new_fg;
            cache.dirty = false;
            mc_dbg_295_ui_draw_cnt = mc_dbg_295_ui_draw_cnt + 1U;
        }
    }

    g_current_field_idx = static_cast<uint8_t>((g_current_field_idx + 1U) % kFieldCount);
}

}  // namespace

void mc_ui_init() {
    mc_dbg_293_ui_request = 0U;
    mc_dbg_294_ui_state = 1U;   // 1 = منتظر نتیجه اسپلش
    mc_dbg_295_ui_draw_cnt = 0U;
    mc_dbg_296_ui_max_tick_us = 0U;
    mc_dbg_297_ui_fake_on = 0U;
    mc_dbg_298_ui_fake_noz_x10 = 0;
    mc_dbg_299_ui_fake_bed_x10 = 0;
    mc_dbg_300_ui_fake_noz_tgt_x10 = 0;
    mc_dbg_301_ui_fake_bed_tgt_x10 = 0;
    mc_dbg_302_ui_fake_status = 0U;
    mc_dbg_303_ui_fake_media = 0U;
    mc_dbg_304_ui_fake_fan_pct = 0U;
    mc_dbg_305_ui_fake_axis_en = 0U;
    mc_dbg_306_ui_media_shown = 0U;
    mc_dbg_307_ui_fake_axis_dir = 0U;
    mc_dbg_308_ui_dir_shown = 0U;
    mc_dbg_311_ui_fake_src = 0U;
    mc_dbg_312_ui_src_shown = 0U;
    mc_dbg_318_ui_axis_white_shown = 0U;
    mc_dbg_319_ui_sel_shown = 0U;
    mc_dbg_320_ui_enter_cnt = 0U;
    mc_dbg_321_ui_enter_idx = 255U;
    mc_dbg_322_ui_sel_force = 0U;
    mc_dbg_323_ui_page = 0U;
    mc_dbg_324_menu_sel_shown = 0U;
    mc_dbg_325_menu_first_shown = 0U;
    mc_dbg_326_menu_enter_cnt = 0U;
    mc_dbg_327_menu_enter_idx = 255U;
    mc_dbg_328_menu_force = 0U;
    mc_dbg_329_move_sel_shown = 12U;
    mc_dbg_330_move_enter_cnt = 0U;
    mc_dbg_331_move_enter_idx = 255U;
    mc_dbg_332_move_force = 0U;
    mc_dbg_333_move_step_shown = 1U;
    mc_dbg_340_bed_sel_shown = 3U;
    mc_dbg_341_bed_enter_cnt = 0U;
    mc_dbg_342_bed_enter_idx = 255U;
    mc_dbg_343_bed_force = 0U;
    mc_dbg_344_sen_sel_shown = 4U;
    mc_dbg_345_sen_enter_cnt = 0U;
    mc_dbg_346_sen_enter_idx = 255U;
    mc_dbg_347_sen_force = 0U;
    mc_dbg_348_sen_fake = 0U;
    mc_dbg_349_sd_sel_shown = 2U;
    mc_dbg_350_sd_enter_cnt = 0U;
    mc_dbg_351_sd_enter_idx = 255U;
    mc_dbg_352_sd_force = 0U;
    mc_dbg_353_sd_fake = 0U;
    mc_dbg_354_otg_sel_shown = 2U;
    mc_dbg_355_otg_enter_cnt = 0U;
    mc_dbg_356_otg_enter_idx = 255U;
    mc_dbg_357_otg_force = 0U;
    mc_dbg_358_otg_fake = 0U;
    mc_dbg_359_ui_fake_print_pct = 0U;
    mc_dbg_360_about_sel_shown = 0U;
    mc_dbg_361_about_force = 0U;

    for (uint8_t i = 0U; i < kFieldCount; ++i) {
        g_field_cache[i].text[0] = '\0';
        g_field_cache[i].fg_color = 0xFFFFU;
        g_field_cache[i].state = 0xFFU;
        g_field_cache[i].dirty = true;
    }

    g_splash_hold_start_ms = 0U;
    g_static_frame_step = 0U;
    g_current_field_idx = 0U;
}

void mc_ui_notify_splash(uint32_t result) {
    if ((result == 2U) || (result == 4U)) {
        mc_dbg_294_ui_state = 5U;   // 5 = غیرفعال (نمایشگر ناموجود/خطا)
        mc_log(LogLevel::Warn, "ui disabled: display unavailable");
    } else {
        g_splash_hold_start_ms = mc_millis();
        mc_dbg_294_ui_state = 2U;   // 2 = مکث بعد از اسپلش
    }
}

void mc_ui_poll() {
    const uint32_t start_us = mc_micros();

    // تماس با همه متغیرهای دیباگ جهت جلوگیری از حذف توسط لینکر
    (void)mc_dbg_297_ui_fake_on;
    (void)mc_dbg_298_ui_fake_noz_x10;
    (void)mc_dbg_299_ui_fake_bed_x10;
    (void)mc_dbg_300_ui_fake_noz_tgt_x10;
    (void)mc_dbg_301_ui_fake_bed_tgt_x10;
    (void)mc_dbg_302_ui_fake_status;
    (void)mc_dbg_303_ui_fake_media;
    (void)mc_dbg_304_ui_fake_fan_pct;
    (void)mc_dbg_305_ui_fake_axis_en;
    (void)mc_dbg_306_ui_media_shown;
    (void)mc_dbg_307_ui_fake_axis_dir;
    (void)mc_dbg_308_ui_dir_shown;
    (void)mc_dbg_311_ui_fake_src;
    (void)mc_dbg_312_ui_src_shown;
    (void)mc_dbg_318_ui_axis_white_shown;
    (void)mc_dbg_319_ui_sel_shown;
    (void)mc_dbg_320_ui_enter_cnt;
    (void)mc_dbg_321_ui_enter_idx;
    (void)mc_dbg_322_ui_sel_force;
    (void)mc_dbg_323_ui_page;
    (void)mc_dbg_324_menu_sel_shown;
    (void)mc_dbg_325_menu_first_shown;
    (void)mc_dbg_326_menu_enter_cnt;
    (void)mc_dbg_327_menu_enter_idx;
    (void)mc_dbg_328_menu_force;
    (void)mc_dbg_329_move_sel_shown;
    (void)mc_dbg_330_move_enter_cnt;
    (void)mc_dbg_331_move_enter_idx;
    (void)mc_dbg_332_move_force;
    (void)mc_dbg_333_move_step_shown;
    (void)mc_dbg_334_ex_sel_shown;
    (void)mc_dbg_335_ex_enter_cnt;
    (void)mc_dbg_336_ex_enter_idx;
    (void)mc_dbg_337_ex_force;
    (void)mc_dbg_338_ex_feed_shown;
    (void)mc_dbg_339_ex_fake_rule;
    (void)mc_dbg_340_bed_sel_shown;
    (void)mc_dbg_341_bed_enter_cnt;
    (void)mc_dbg_342_bed_enter_idx;
    (void)mc_dbg_343_bed_force;
    (void)mc_dbg_344_sen_sel_shown;
    (void)mc_dbg_345_sen_enter_cnt;
    (void)mc_dbg_346_sen_enter_idx;
    (void)mc_dbg_347_sen_force;
    (void)mc_dbg_348_sen_fake;
    (void)mc_dbg_349_sd_sel_shown;
    (void)mc_dbg_350_sd_enter_cnt;
    (void)mc_dbg_351_sd_enter_idx;
    (void)mc_dbg_352_sd_force;
    (void)mc_dbg_353_sd_fake;
    (void)mc_dbg_354_otg_sel_shown;
    (void)mc_dbg_355_otg_enter_cnt;
    (void)mc_dbg_356_otg_enter_idx;
    (void)mc_dbg_357_otg_force;
    (void)mc_dbg_358_otg_fake;
    (void)mc_dbg_359_ui_fake_print_pct;
    (void)mc_dbg_360_about_sel_shown;
    (void)mc_dbg_361_about_force;
    (void)mc_dbg_362_touch_cal_force;
    (void)mc_dbg_370_touch_cal_src;
    (void)mc_dbg_371_touch_save_result;

    // بررسی تغییر اجباری وضعیت کالیبراسیون با متغیر دیباگ 362 (بند ۸.۱۷)
    if (mc_dbg_362_touch_cal_force != 0U) {
        const uint32_t f_cal = mc_dbg_362_touch_cal_force;
        mc_dbg_362_touch_cal_force = 0U;
        if (f_cal == 1U) {
            if (mc_dbg_294_ui_state != 5U) {
                mc_dbg_323_ui_page = 9U;
                mc_ui_tcal_open();
            }
        } else if (f_cal == 2U) {
            if (mc_dbg_323_ui_page == 9U) {
                mc_dbg_323_ui_page = 0U;
                mc_dbg_293_ui_request = 1U;
            }
        } else if (f_cal == 3U) {
            (void)mc_tcal_store_clear_start();
        }
    }

    // بارگذاری خودکار کالیبراسیون لمس هنگام بوت بعد از پایان خواندن تنظیمات
    static bool g_boot_touch_loaded = false;
    static bool g_boot_touch_started = false;
    if ((!g_boot_touch_loaded) && (!mc_cfg_boot_busy()) && (mc_dbg_294_ui_state == 4U)) {
        if (!g_boot_touch_started) {
            if (mc_tcal_store_load_start()) {
                g_boot_touch_started = true;
            }
        } else {
            mc_tcal_store_poll();
            const uint8_t st = mc_tcal_store_state();
            if (st != 1U) {   // کار بارگذاری تمام شد
                g_boot_touch_loaded = true;
                const uint8_t res = mc_tcal_store_result();
                if (res == 1U) {
                    mc_dbg_370_touch_cal_src = 1U;   // از EEPROM بارگذاری شد
                    if (g_enc_pressed_at_boot) {
                        mc_dbg_323_ui_page = 9U;
                        mc_ui_tcal_open();
                    }
                } else {
                    mc_dbg_370_touch_cal_src = 0U;   // کالیبره نیست
                    mc_dbg_323_ui_page = 9U;
                    mc_ui_tcal_open();
                }
            }
        }
    }

    // بررسی درخواست نقاشی مجدد
    if (mc_dbg_293_ui_request != 0U) {
        mc_dbg_293_ui_request = 0U;
        mc_dbg_323_ui_page = 0U;   // بازگشت به صفحه اصلی
        if (mc_dbg_294_ui_state != 5U) {
            g_static_frame_step = 0U;
            mc_dbg_294_ui_state = 3U;   // نقاشی مستقیم قاب ثابت
        }
    }


    // اگر صفحه فعال منو باشد (۱)
    if (mc_dbg_323_ui_page == 1U) {
        const MenuResult m_res = mc_ui_menu_poll();
        if (m_res == MenuResult::Back) {
            mc_dbg_323_ui_page = 0U;
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
            if (mc_dbg_294_ui_state != 5U) {
                g_static_frame_step = 0U;
                mc_dbg_294_ui_state = 3U;     // بازنقاشی کامل صفحه اصلی
                mc_dbg_319_ui_sel_shown = 4U; // هایلایت روی گزینه منو (اندیس ۴)
            }
        } else if (m_res == MenuResult::OpenMove) {
            mc_dbg_323_ui_page = 2U;
            mc_ui_move_open();
        } else if (m_res == MenuResult::OpenExtruder) {
            mc_dbg_323_ui_page = 3U;
            mc_ui_extruder_open();
        } else if (m_res == MenuResult::OpenBed) {
            mc_dbg_323_ui_page = 4U;
            mc_ui_bed_open();
        } else if (m_res == MenuResult::OpenSensors) {
            mc_dbg_323_ui_page = 5U;
            mc_ui_sensors_open();
        } else if (m_res == MenuResult::OpenSd) {
            mc_dbg_323_ui_page = 6U;
            mc_ui_sd_open();
        } else if (m_res == MenuResult::OpenOtg) {
            mc_dbg_323_ui_page = 7U;
            mc_ui_otg_open();
        } else if (m_res == MenuResult::OpenAbout) {
            mc_dbg_323_ui_page = 8U;
            mc_ui_about_open();
        }
        const uint32_t elapsed_us = mc_micros() - start_us;
        if (elapsed_us > mc_dbg_296_ui_max_tick_us) {
            mc_dbg_296_ui_max_tick_us = elapsed_us;
        }
        return;
    }

    // اگر صفحه فعال صفحه حرکت باشد (۲)
    if (mc_dbg_323_ui_page == 2U) {
        const MoveResult mv_res = mc_ui_move_poll();
        if (mv_res == MoveResult::Back) {
            mc_dbg_323_ui_page = 1U;
            mc_ui_menu_open_at(0U);
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
        }
        const uint32_t elapsed_us = mc_micros() - start_us;
        if (elapsed_us > mc_dbg_296_ui_max_tick_us) {
            mc_dbg_296_ui_max_tick_us = elapsed_us;
        }
        return;
    }

    // اگر صفحه فعال صفحه اکسترودر باشد (۳)
    if (mc_dbg_323_ui_page == 3U) {
        const ExtruderResult ex_res = mc_ui_extruder_poll();
        if (ex_res == ExtruderResult::Back) {
            mc_dbg_323_ui_page = 1U;
            mc_ui_menu_open_at(1U);
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
        }
        const uint32_t elapsed_us = mc_micros() - start_us;
        if (elapsed_us > mc_dbg_296_ui_max_tick_us) {
            mc_dbg_296_ui_max_tick_us = elapsed_us;
        }
        return;
    }

    // اگر صفحه فعال صفحه داغ باشد (۴)
    if (mc_dbg_323_ui_page == 4U) {
        const BedResult b_res = mc_ui_bed_poll();
        if (b_res == BedResult::Back) {
            mc_dbg_323_ui_page = 1U;
            mc_ui_menu_open_at(2U);
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
        }
        const uint32_t elapsed_us = mc_micros() - start_us;
        if (elapsed_us > mc_dbg_296_ui_max_tick_us) {
            mc_dbg_296_ui_max_tick_us = elapsed_us;
        }
        return;
    }

    // اگر صفحه فعال صفحه سنسورها باشد (۵)
    if (mc_dbg_323_ui_page == 5U) {
        const SensorsResult s_res = mc_ui_sensors_poll();
        if (s_res == SensorsResult::Back) {
            mc_dbg_323_ui_page = 1U;
            mc_ui_menu_open_at(3U);
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
        }
        const uint32_t elapsed_us = mc_micros() - start_us;
        if (elapsed_us > mc_dbg_296_ui_max_tick_us) {
            mc_dbg_296_ui_max_tick_us = elapsed_us;
        }
        return;
    }

    // اگر صفحه فعال صفحه SD CARD باشد (۶)
    if (mc_dbg_323_ui_page == 6U) {
        const SdResult sd_res = mc_ui_sd_poll();
        if (sd_res == SdResult::Back) {
            mc_dbg_323_ui_page = 1U;
            mc_ui_menu_open_at(4U);
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
        }
        const uint32_t elapsed_us = mc_micros() - start_us;
        if (elapsed_us > mc_dbg_296_ui_max_tick_us) {
            mc_dbg_296_ui_max_tick_us = elapsed_us;
        }
        return;
    }

    // اگر صفحه فعال صفحه USB OTG باشد (۷)
    if (mc_dbg_323_ui_page == 7U) {
        const OtgResult otg_res = mc_ui_otg_poll();
        if (otg_res == OtgResult::Back) {
            mc_dbg_323_ui_page = 1U;
            mc_ui_menu_open_at(5U);
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
        }
        const uint32_t elapsed_us = mc_micros() - start_us;
        if (elapsed_us > mc_dbg_296_ui_max_tick_us) {
            mc_dbg_296_ui_max_tick_us = elapsed_us;
        }
        return;
    }

    // اگر صفحه فعال صفحه ABOUT باشد (۸)
    if (mc_dbg_323_ui_page == 8U) {
        const AboutResult ab_res = mc_ui_about_poll();
        if (ab_res == AboutResult::Back) {
            mc_dbg_323_ui_page = 1U;
            mc_ui_menu_open_at(6U);
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
        }
        const uint32_t elapsed_us = mc_micros() - start_us;
        if (elapsed_us > mc_dbg_296_ui_max_tick_us) {
            mc_dbg_296_ui_max_tick_us = elapsed_us;
        }
        return;
    }

    // اگر صفحه فعال صفحه TOUCH CAL باشد (۹) (بند ۸.۱۷)
    if (mc_dbg_323_ui_page == 9U) {
        const TcalResult tc_res = mc_ui_tcal_poll();
        if (tc_res != TcalResult::Stay) {
            mc_dbg_323_ui_page = 0U;
            mc_dbg_293_ui_request = 1U;
            (void)mc_encoder_take_clicks();
            (void)mc_encoder_take_press();
        }
        const uint32_t elapsed_us = mc_micros() - start_us;
        if (elapsed_us > mc_dbg_296_ui_max_tick_us) {
            mc_dbg_296_ui_max_tick_us = elapsed_us;
        }
        return;
    }


    // اگر صفحه فعال 0 یا 1 باشد، بررسی mc_dbg_332_move_force جهت ورود مستقیم به صفحه حرکت
    if (mc_dbg_332_move_force != 0U) {
        const uint32_t f_move = mc_dbg_332_move_force;
        if ((f_move == 1U) || ((f_move >= 3U) && (f_move <= 15U))) {
            if (f_move == 1U) {
                mc_dbg_332_move_force = 0U;
            }
            mc_ui_move_open();
            mc_dbg_323_ui_page = 2U;
            (void)mc_ui_move_poll();
            const uint32_t elapsed_us = mc_micros() - start_us;
            if (elapsed_us > mc_dbg_296_ui_max_tick_us) {
                mc_dbg_296_ui_max_tick_us = elapsed_us;
            }
            return;
        } else if (f_move == 2U) {
            mc_dbg_332_move_force = 0U;
        }
    }

    // اگر صفحه فعال 0 یا 1 باشد، بررسی mc_dbg_337_ex_force جهت ورود مستقیم به صفحه اکسترودر
    if (mc_dbg_337_ex_force != 0U) {
        const uint32_t f_ex = mc_dbg_337_ex_force;
        if ((f_ex == 1U) || ((f_ex >= 3U) && (f_ex <= 10U))) {
            if (f_ex == 1U) {
                mc_dbg_337_ex_force = 0U;
            }
            mc_ui_extruder_open();
            mc_dbg_323_ui_page = 3U;
            (void)mc_ui_extruder_poll();
            const uint32_t elapsed_us = mc_micros() - start_us;
            if (elapsed_us > mc_dbg_296_ui_max_tick_us) {
                mc_dbg_296_ui_max_tick_us = elapsed_us;
            }
            return;
        } else if (f_ex == 2U) {
            mc_dbg_337_ex_force = 0U;
        }
    }

    // اگر صفحه فعال 0 یا 1 باشد، بررسی mc_dbg_343_bed_force جهت ورود مستقیم به صفحه داغ
    if (mc_dbg_343_bed_force != 0U) {
        const uint32_t f_bed = mc_dbg_343_bed_force;
        if ((f_bed == 1U) || ((f_bed >= 3U) && (f_bed <= 6U))) {
            if (f_bed == 1U) {
                mc_dbg_343_bed_force = 0U;
            }
            mc_ui_bed_open();
            mc_dbg_323_ui_page = 4U;
            (void)mc_ui_bed_poll();
            const uint32_t elapsed_us = mc_micros() - start_us;
            if (elapsed_us > mc_dbg_296_ui_max_tick_us) {
                mc_dbg_296_ui_max_tick_us = elapsed_us;
            }
            return;
        } else if (f_bed == 2U) {
            mc_dbg_343_bed_force = 0U;
        }
    }

    // اگر صفحه فعال 0 یا 1 باشد، بررسی mc_dbg_347_sen_force جهت ورود مستقیم به صفحه سنسورها
    if (mc_dbg_347_sen_force != 0U) {
        const uint32_t f_sen = mc_dbg_347_sen_force;
        if ((f_sen == 1U) || ((f_sen >= 3U) && (f_sen <= 7U))) {
            if (f_sen == 1U) {
                mc_dbg_347_sen_force = 0U;
            }
            mc_ui_sensors_open();
            mc_dbg_323_ui_page = 5U;
            (void)mc_ui_sensors_poll();
            const uint32_t elapsed_us = mc_micros() - start_us;
            if (elapsed_us > mc_dbg_296_ui_max_tick_us) {
                mc_dbg_296_ui_max_tick_us = elapsed_us;
            }
            return;
        } else if (f_sen == 2U) {
            mc_dbg_347_sen_force = 0U;
        }
    }

    // اگر صفحه فعال 0 یا 1 باشد، بررسی mc_dbg_352_sd_force جهت ورود مستقیم به صفحه SD CARD
    if (mc_dbg_352_sd_force != 0U) {
        const uint32_t f_sd = mc_dbg_352_sd_force;
        if ((f_sd == 1U) || ((f_sd >= 3U) && (f_sd <= 5U))) {
            if (f_sd == 1U) {
                mc_dbg_352_sd_force = 0U;
            }
            mc_ui_sd_open();
            mc_dbg_323_ui_page = 6U;
            (void)mc_ui_sd_poll();
            const uint32_t elapsed_us = mc_micros() - start_us;
            if (elapsed_us > mc_dbg_296_ui_max_tick_us) {
                mc_dbg_296_ui_max_tick_us = elapsed_us;
            }
            return;
        } else if (f_sd == 2U) {
            mc_dbg_352_sd_force = 0U;
        }
    }

    // اگر صفحه فعال 0 یا 1 باشد، بررسی mc_dbg_357_otg_force جهت ورود مستقیم به صفحه USB OTG
    if (mc_dbg_357_otg_force != 0U) {
        const uint32_t f_otg = mc_dbg_357_otg_force;
        if ((f_otg == 1U) || ((f_otg >= 3U) && (f_otg <= 5U))) {
            if (f_otg == 1U) {
                mc_dbg_357_otg_force = 0U;
            }
            mc_ui_otg_open();
            mc_dbg_323_ui_page = 7U;
            (void)mc_ui_otg_poll();
            const uint32_t elapsed_us = mc_micros() - start_us;
            if (elapsed_us > mc_dbg_296_ui_max_tick_us) {
                mc_dbg_296_ui_max_tick_us = elapsed_us;
            }
            return;
        } else if (f_otg == 2U) {
            mc_dbg_357_otg_force = 0U;
        }
    }

    // اگر صفحه فعال 0 یا 1 باشد، بررسی mc_dbg_361_about_force جهت ورود مستقیم به صفحه ABOUT
    if (mc_dbg_361_about_force != 0U) {
        const uint32_t f_about = mc_dbg_361_about_force;
        if (f_about == 1U) {
            mc_dbg_361_about_force = 0U;
            mc_ui_about_open();
            mc_dbg_323_ui_page = 8U;
            (void)mc_ui_about_poll();
            const uint32_t elapsed_us = mc_micros() - start_us;
            if (elapsed_us > mc_dbg_296_ui_max_tick_us) {
                mc_dbg_296_ui_max_tick_us = elapsed_us;
            }
            return;
        } else if (f_about == 2U) {
            mc_dbg_361_about_force = 0U;
        }
    }

    // اگر صفحه فعال صفحه اصلی باشد (۰)، بررسی mc_dbg_328_menu_force جهت ورود به منو
    if (mc_dbg_328_menu_force != 0U) {
        const uint32_t f_val = mc_dbg_328_menu_force;
        if ((f_val == 1U) || ((f_val >= 3U) && (f_val <= 10U))) {
            mc_ui_menu_open();
            mc_dbg_323_ui_page = 1U;
            (void)mc_ui_menu_poll();
            const uint32_t elapsed_us = mc_micros() - start_us;
            if (elapsed_us > mc_dbg_296_ui_max_tick_us) {
                mc_dbg_296_ui_max_tick_us = elapsed_us;
            }
            return;
        } else if (f_val == 2U) {
            mc_dbg_328_menu_force = 0U;
        }
    }

    switch (mc_dbg_294_ui_state) {
        case 2: { // مکث بعد از اسپلش
            if (mc_time_elapsed_ms(g_splash_hold_start_ms, MC_UI_SPLASH_HOLD_MS)) {
                g_static_frame_step = 0U;
                mc_dbg_294_ui_state = 3U;   // رفتن به نقاشی قاب ثابت
            }
            break;
        }

        case 3: { // نقاشی مرحله به مرحله قاب ثابت
            if (g_static_frame_step < 16U) {
                draw_static_step(g_static_frame_step);
                g_static_frame_step++;
                if (g_static_frame_step >= 16U) {
                    for (uint8_t i = 0U; i < kFieldCount; ++i) {
                        g_field_cache[i].dirty = true;
                        g_field_cache[i].text[0] = '\0';
                        g_field_cache[i].fg_color = 0xFFFFU;
                        g_field_cache[i].state = 0xFFU;
                    }
                    g_current_field_idx = 0U;
                    mc_dbg_294_ui_state = 4U;   // کار عادی

                    // بند ۸.۲: هنگام ورود به کار عادی/بازنقاشی کامل، کلیک‌ها و اینترهای قدیمی دور ریخته می‌شوند و هایلایت گزینه فعلی کشیده می‌شود
                    (void)mc_encoder_take_clicks();
                    (void)mc_encoder_take_press();
                    if (mc_dbg_319_ui_sel_shown >= 5U) {
                        mc_dbg_319_ui_sel_shown = 0U;
                    }
                    draw_option_highlight(static_cast<uint8_t>(mc_dbg_319_ui_sel_shown), kColorWhite);
                }
            }
            break;
        }

        case 4: { // کار عادی (پردازش انکودر + به‌روزرسانی یک فیلد در هر دور)
            // ۱. بررسی انتخاب اجباری با متغیر دیباگ mc_dbg_322_ui_sel_force (۱ تا ۵)
            if ((mc_dbg_322_ui_sel_force >= 1U) && (mc_dbg_322_ui_sel_force <= 5U)) {
                const uint32_t target_idx = mc_dbg_322_ui_sel_force - 1U;
                mc_dbg_322_ui_sel_force = 0U;
                if (target_idx != mc_dbg_319_ui_sel_shown) {
                    draw_option_highlight(static_cast<uint8_t>(mc_dbg_319_ui_sel_shown),
                                          kOptionRects[mc_dbg_319_ui_sel_shown].bg_color);
                    mc_dbg_319_ui_sel_shown = target_idx;
                    draw_option_highlight(static_cast<uint8_t>(mc_dbg_319_ui_sel_shown), kColorWhite);
                }
            }

            // ۲. خواندن کلیک‌های چرخش انکودر و تغییر گزینه‌ی هایلایت‌شده
            const int32_t clicks = mc_encoder_take_clicks();
            if (clicks != 0) {
                int32_t new_idx = static_cast<int32_t>(mc_dbg_319_ui_sel_shown) + clicks;
                while (new_idx < 0) {
                    new_idx += 5;
                }
                while (new_idx >= 5) {
                    new_idx %= 5;
                }
                const uint32_t new_sel = static_cast<uint32_t>(new_idx);
                if (new_sel != mc_dbg_319_ui_sel_shown) {
                    draw_option_highlight(static_cast<uint8_t>(mc_dbg_319_ui_sel_shown),
                                          kOptionRects[mc_dbg_319_ui_sel_shown].bg_color);
                    mc_dbg_319_ui_sel_shown = new_sel;
                    draw_option_highlight(static_cast<uint8_t>(mc_dbg_319_ui_sel_shown), kColorWhite);
                }
            }

            // ۳. خواندن فشردن دکمه انکودر (اینتر)
            if (mc_encoder_take_press()) {
                mc_dbg_320_ui_enter_cnt = mc_dbg_320_ui_enter_cnt + 1U;
                mc_dbg_321_ui_enter_idx = mc_dbg_319_ui_sel_shown;
                if (mc_dbg_319_ui_sel_shown == 4U) { // اینتر روی گزینه منو
                    mc_ui_menu_open();
                    mc_dbg_323_ui_page = 1U;
                }
            }

            // ۴. به‌روزرسانی فیلدهای متغیر
            update_one_field();
            break;
        }

        default:
            break;
    }

    const uint32_t elapsed_us = mc_micros() - start_us;
    if (elapsed_us > mc_dbg_296_ui_max_tick_us) {
        mc_dbg_296_ui_max_tick_us = elapsed_us;
    }
}

}  // namespace mc
