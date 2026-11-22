// صفحه اصلی رابط کاربری (بند ۸.۶ و ۸.۷).
// هیچ هدر ST اینجا include نمی‌شود.
#include "ui_home.h"
#include "ui_icons.h"
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
#include "../config/mc_version.h"
#include "../hal/hal_stepper.h"
#include "../hal/hal_display.h"
#include "../hal/hal_time.h"

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
}

namespace mc {
namespace {

constexpr uint16_t kTopBarBg    = mc_rgb565(0, 40, 90);
constexpr uint16_t kCardBg      = mc_rgb565(24, 28, 36);      // پس‌زمینه کارت‌ها و دکمه‌ها
constexpr uint16_t kIconOff     = mc_rgb565(100, 120, 160);   // آیکون SD/OTG وقتی موجود نیست
constexpr uint16_t kCoolBlue    = mc_rgb565(100, 180, 255);   // حالت COOL
constexpr uint16_t kCyanPrint   = mc_rgb565(0, 220, 220);     // حالت PRINTING (آینده)
constexpr uint16_t kLabelGray   = mc_rgb565(180, 180, 180);
constexpr uint16_t kDividerGray = mc_rgb565(80, 80, 80);
constexpr uint16_t kYellowBusy  = mc_rgb565(255, 220, 0);
constexpr uint16_t kOrangeTgt   = mc_rgb565(255, 140, 0);

struct FieldSpec {
    uint16_t x;
    uint16_t y;
    uint8_t  width;   // تعداد کاراکتر
    uint8_t  scale;
    uint16_t bg_color;
};

static const FieldSpec kFields[19] = {
    {  10U,   8U,  0U, 0U, kTopBarBg },  //  0: SD Icon
    {  46U,   8U,  0U, 0U, kTopBarBg },  //  1: USB Icon
    {  44U,  66U,  5U, 2U, kCardBg },    //  2: Extruder Status
    {  14U,  90U,  5U, 2U, kCardBg },    //  3: Extruder Current Temp
    {  86U,  90U,  3U, 2U, kCardBg },    //  4: Extruder Target Temp
    { 200U,  66U,  5U, 2U, kCardBg },    //  5: Fan Status
    { 170U,  90U,  4U, 2U, kCardBg },    //  6: Fan Percent
    { 356U,  66U,  5U, 2U, kCardBg },    //  7: Bed Status
    { 326U,  90U,  5U, 2U, kCardBg },    //  8: Bed Current Temp
    { 398U,  90U,  3U, 2U, kCardBg },    //  9: Bed Target Temp
    {  40U, 126U,  3U, 2U, kCardBg },    // 10: Axis X Status (special LED)
    { 160U, 126U,  3U, 2U, kCardBg },    // 11: Axis Y Status (special LED)
    { 280U, 126U,  3U, 2U, kCardBg },    // 12: Axis Z Status (special LED)
    { 400U, 126U,  3U, 2U, kCardBg },    // 13: Axis E Status (special LED)
    {  16U, 152U,  8U, 2U, kCardBg },    // 14: Axis X Position
    { 136U, 152U,  8U, 2U, kCardBg },    // 15: Axis Y Position
    { 256U, 152U,  8U, 2U, kCardBg },    // 16: Axis Z Position
    { 376U, 152U,  8U, 2U, kCardBg },    // 17: Axis E Position
    { 150U, 190U, 10U, 3U, kColorBlack } // 18: Printer Status
};

struct FieldCache {
    char     text[16];
    uint16_t fg_color;
    uint8_t  state;
    bool     dirty;
};

static FieldCache g_field_cache[19];
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

// وسط‌چین کردن متن در عرض target_width با فاصله (space)
void center_and_copy(char* dst, uint16_t cap, const char* src, uint8_t target_width) {
    uint16_t len = 0U;
    while ((src[len] != '\0') && (len < target_width)) {
        len++;
    }
    const uint8_t total_pad = (target_width > len) ? static_cast<uint8_t>(target_width - len) : 0U;
    const uint8_t left_pad = static_cast<uint8_t>((total_pad + 1U) / 2U);

    uint16_t pos = 0U;
    for (uint8_t i = 0U; (i < left_pad) && (pos + 1U < cap); ++i) {
        dst[pos++] = ' ';
    }
    for (uint16_t i = 0U; (i < len) && (pos + 1U < cap); ++i) {
        dst[pos++] = src[i];
    }
    while ((pos < target_width) && (pos + 1U < cap)) {
        dst[pos++] = ' ';
    }
    dst[pos] = '\0';
}

// تولید متن و رنگ فیلدهای متنی (۲ تا ۹، ۱۴ تا ۱۸)
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

        case 5: { // وضعیت فن
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

        case 6: { // درصد فن
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
            pad_and_copy(out_buf, cap, raw, spec.width);
            break;
        }

        case 7: { // وضعیت بستر
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

        case 8: { // دمای فعلی بستر
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

        case 9: { // هدف بستر
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

        case 14:
        case 15:
        case 16:
        case 17: { // موقعیت محورها X، Y، Z، E
            const uint8_t axis_code = idx - 14U;
            const float pos = mc_position_get(axis_code);
            mc_fmt_float(raw, sizeof(raw), 0U, pos, 2U);
            out_fg = kColorWhite;
            pad_and_copy(out_buf, cap, raw, spec.width);
            break;
        }

        case 18: { // وضعیت پرینتر
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
            center_and_copy(out_buf, cap, raw, spec.width);
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
            mc_display_fill_screen(kColorBlack);
            break;
        case 1:
            mc_gfx_fill_rect(0U, 0U, 480U, 40U, kTopBarBg);
            break;
        case 2:
            mc_icon_menu(446U, 8U, kColorWhite);
            break;
        case 3: {
            char dev_name[25];
            mc_fmt_text(dev_name, sizeof(dev_name), 0U, MC_DEVICE_NAME);
            const uint16_t tw = mc_gfx_text_width(dev_name, 2U);
            const uint16_t nx = (tw < 480U) ? static_cast<uint16_t>((480U - tw) / 2U) : 0U;
            mc_gfx_text(nx, 12U, dev_name, kColorWhite, kTopBarBg, 2U);
            break;
        }
        case 4:
            mc_gfx_fill_rect(8U, 48U, 152U, 64U, kCardBg);
            mc_gfx_rect(8U, 48U, 152U, 64U, kDividerGray);
            mc_icon_nozzle(14U, 52U, kColorWhite);
            mc_gfx_text(44U, 54U, "NOZZLE", kLabelGray, kCardBg, 1U);
            mc_gfx_text(74U, 90U, "/", kColorWhite, kCardBg, 2U);
            mc_gfx_text(122U, 90U, "C", kColorWhite, kCardBg, 2U);
            break;
        case 5:
            mc_gfx_fill_rect(164U, 48U, 152U, 64U, kCardBg);
            mc_gfx_rect(164U, 48U, 152U, 64U, kDividerGray);
            mc_icon_fan(170U, 52U, kColorWhite, kCardBg);
            mc_gfx_text(200U, 54U, "FAN", kLabelGray, kCardBg, 1U);
            break;
        case 6:
            mc_gfx_fill_rect(320U, 48U, 152U, 64U, kCardBg);
            mc_gfx_rect(320U, 48U, 152U, 64U, kDividerGray);
            mc_icon_bed(326U, 52U, kColorWhite);
            mc_gfx_text(356U, 54U, "BED", kLabelGray, kCardBg, 1U);
            mc_gfx_text(386U, 90U, "/", kColorWhite, kCardBg, 2U);
            mc_gfx_text(434U, 90U, "C", kColorWhite, kCardBg, 2U);
            break;
        case 7:
            mc_gfx_fill_rect(8U, 120U, 112U, 56U, kCardBg);
            mc_gfx_rect(8U, 120U, 112U, 56U, kDividerGray);
            mc_gfx_text(16U, 126U, "X", kLabelGray, kCardBg, 2U);
            break;
        case 8:
            mc_gfx_fill_rect(128U, 120U, 112U, 56U, kCardBg);
            mc_gfx_rect(128U, 120U, 112U, 56U, kDividerGray);
            mc_gfx_text(136U, 126U, "Y", kLabelGray, kCardBg, 2U);
            break;
        case 9:
            mc_gfx_fill_rect(248U, 120U, 112U, 56U, kCardBg);
            mc_gfx_rect(248U, 120U, 112U, 56U, kDividerGray);
            mc_gfx_text(256U, 126U, "Z", kLabelGray, kCardBg, 2U);
            break;
        case 10:
            mc_gfx_fill_rect(368U, 120U, 112U, 56U, kCardBg);
            mc_gfx_rect(368U, 120U, 112U, 56U, kDividerGray);
            mc_gfx_text(376U, 126U, "E", kLabelGray, kCardBg, 2U);
            break;
        case 11:
            mc_gfx_rect(140U, 184U, 200U, 36U, kDividerGray);
            break;
        case 12:
            mc_gfx_fill_rect(8U, 228U, 104U, 64U, kCardBg);
            mc_gfx_rect(8U, 228U, 104U, 64U, kDividerGray);
            mc_icon_play(44U, 244U, kColorGreen);
            break;
        case 13:
            mc_gfx_fill_rect(128U, 228U, 104U, 64U, kCardBg);
            mc_gfx_rect(128U, 228U, 104U, 64U, kDividerGray);
            mc_icon_pause(164U, 244U, kYellowBusy);
            break;
        case 14:
            mc_gfx_fill_rect(248U, 228U, 104U, 64U, kCardBg);
            mc_gfx_rect(248U, 228U, 104U, 64U, kDividerGray);
            mc_icon_stop(284U, 244U, kColorRed);
            break;
        case 15:
            mc_gfx_fill_rect(368U, 228U, 104U, 64U, kCardBg);
            mc_gfx_rect(368U, 228U, 104U, 64U, kDividerGray);
            mc_icon_reset(404U, 244U, kOrangeTgt);
            break;
        case 16:
            mc_gfx_text(8U, 304U, MC_FIRMWARE_NAME " " MC_VERSION_STRING, kDividerGray, kColorBlack, 1U);
            break;
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
    } else if (idx >= 10U && idx <= 13U) { // وضعیت و LED محورهای X، Y، Z، E
        char raw[16];
        char new_text[16];
        uint16_t new_fg = kColorWhite;

        const uint8_t axis_idx = idx - 10U;
        const StepAxis axis = static_cast<StepAxis>(axis_idx);
        const bool fake = (mc_dbg_297_ui_fake_on != 0U);
        const bool enabled = fake ? ((mc_dbg_305_ui_fake_axis_en & (1U << axis_idx)) != 0U)
                                  : mc_stepper_is_enabled(axis);

        if (enabled) {
            mc_fmt_text(raw, sizeof(raw), 0U, "ON");
            new_fg = kColorGreen;
        } else {
            mc_fmt_text(raw, sizeof(raw), 0U, "OFF");
            new_fg = kLabelGray;
        }
        pad_and_copy(new_text, sizeof(new_text), raw, spec.width);

        const uint8_t state = enabled ? 1U : 0U;
        const uint16_t led_color = enabled ? kColorGreen : kDividerGray;
        const uint16_t led_cx = static_cast<uint16_t>(spec.x + 64U);

        if (cache.dirty || (cache.fg_color != new_fg) || (cache.state != state) || (strcmp(cache.text, new_text) != 0)) {
            mc_gfx_text(spec.x, spec.y, new_text, new_fg, spec.bg_color, spec.scale);
            mc_ui_fill_circle(led_cx, 134U, 6U, led_color);
            for (uint16_t i = 0U; i < sizeof(cache.text); ++i) {
                cache.text[i] = new_text[i];
                if (new_text[i] == '\0') { break; }
            }
            cache.fg_color = new_fg;
            cache.state = state;
            cache.dirty = false;
            mc_dbg_295_ui_draw_cnt = mc_dbg_295_ui_draw_cnt + 1U;
        }
    } else { // سایر فیلدهای متنی
        char new_text[16];
        uint16_t new_fg = kColorWhite;
        format_field(idx, new_text, sizeof(new_text), new_fg);

        if (cache.dirty || (cache.fg_color != new_fg) || (strcmp(cache.text, new_text) != 0)) {
            mc_gfx_text(spec.x, spec.y, new_text, new_fg, spec.bg_color, spec.scale);
            for (uint16_t i = 0U; i < sizeof(cache.text); ++i) {
                cache.text[i] = new_text[i];
                if (new_text[i] == '\0') { break; }
            }
            cache.fg_color = new_fg;
            cache.dirty = false;
            mc_dbg_295_ui_draw_cnt = mc_dbg_295_ui_draw_cnt + 1U;
        }
    }

    g_current_field_idx = static_cast<uint8_t>((g_current_field_idx + 1U) % 19U);
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

    for (uint8_t i = 0U; i < 19U; ++i) {
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

    // بررسی درخواست نقاشی مجدد
    if (mc_dbg_293_ui_request != 0U) {
        mc_dbg_293_ui_request = 0U;
        if (mc_dbg_294_ui_state != 5U) {
            g_static_frame_step = 0U;
            mc_dbg_294_ui_state = 3U;   // نقاشی مستقیم قاب ثابت
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
            if (g_static_frame_step < 17U) {
                draw_static_step(g_static_frame_step);
                g_static_frame_step++;
                if (g_static_frame_step >= 17U) {
                    for (uint8_t i = 0U; i < 19U; ++i) {
                        g_field_cache[i].dirty = true;
                        g_field_cache[i].text[0] = '\0';
                        g_field_cache[i].fg_color = 0xFFFFU;
                        g_field_cache[i].state = 0xFFU;
                    }
                    g_current_field_idx = 0U;
                    mc_dbg_294_ui_state = 4U;   // کار عادی
                }
            }
            break;
        }

        case 4: { // کار عادی (به‌روزرسانی یک فیلد در هر دور)
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
