// صفحه اصلی رابط کاربری (بند ۸.۶).
// هیچ هدر ST اینجا include نمی‌شود.
#include "ui_home.h"
#include <string.h>
#include "../core/gfx.h"
#include "../core/text_format.h"
#include "../core/thermistor.h"
#include "../core/heater_ex.h"
#include "../core/heater_bed.h"
#include "../core/machine_state.h"
#include "../core/error.h"
#include "../core/log.h"
#include "../config/mc_version.h"
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
volatile uint32_t mc_dbg_302_ui_fake_status     = 0;  // نوشتنی. ۰ = واقعی، ۱ = READY، ۲ = BUSY، ۳ = KILLED
}

namespace mc {
namespace {

constexpr uint16_t kTopBarBg    = mc_rgb565(0, 40, 90);
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

static const FieldSpec kFields[9] = {
    { 376U,   8U, 8U, 2U, kTopBarBg },   // 0: Status
    { 140U,  48U, 6U, 3U, kColorBlack },  // 1: Extruder Current Temp
    { 276U,  48U, 5U, 3U, kColorBlack },  // 2: Extruder Target Temp
    { 140U,  96U, 6U, 3U, kColorBlack },  // 3: Bed Current Temp
    { 276U,  96U, 5U, 3U, kColorBlack },  // 4: Bed Target Temp
    {  32U, 160U, 8U, 3U, kColorBlack },  // 5: X Position
    { 272U, 160U, 8U, 3U, kColorBlack },  // 6: Y Position
    {  32U, 208U, 8U, 3U, kColorBlack },  // 7: Z Position
    { 272U, 208U, 8U, 3U, kColorBlack }   // 8: E Position
};

struct FieldCache {
    char     text[16];
    uint16_t fg_color;
    bool     dirty;
};

static FieldCache g_field_cache[9];
static uint32_t   g_splash_hold_start_ms = 0U;
static uint8_t    g_static_frame_step    = 0U;
static uint8_t    g_current_field_idx    = 0U;

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

// تولید متن و رنگ یک فیلد
void format_field(uint8_t idx, char* out_buf, uint16_t cap, uint16_t& out_fg) {
    char raw[16];
    raw[0] = '\0';

    const bool fake = (mc_dbg_297_ui_fake_on != 0U);

    switch (idx) {
        case 0: { // وضعیت
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
            } else {
                mc_fmt_text(raw, sizeof(raw), 0U, "READY");
                out_fg = kColorGreen;
            }
            break;
        }

        case 1: { // دمای فعلی اکسترودر
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
            break;
        }

        case 2: { // هدف اکسترودر
            const int32_t tgt_x10 = fake ? mc_dbg_300_ui_fake_noz_tgt_x10 : mc_heater_ex_target_x10();
            if (tgt_x10 <= 0) {
                mc_fmt_text(raw, sizeof(raw), 0U, "0.0");
                out_fg = kLabelGray;
            } else {
                const float val = static_cast<float>(tgt_x10) / 10.0f;
                mc_fmt_float(raw, sizeof(raw), 0U, val, 1U);
                out_fg = kOrangeTgt;
            }
            break;
        }

        case 3: { // دمای فعلی بستر
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
            break;
        }

        case 4: { // هدف بستر
            const int32_t tgt_x10 = fake ? mc_dbg_301_ui_fake_bed_tgt_x10 : mc_heater_bed_target_x10();
            if (tgt_x10 <= 0) {
                mc_fmt_text(raw, sizeof(raw), 0U, "0.0");
                out_fg = kLabelGray;
            } else {
                const float val = static_cast<float>(tgt_x10) / 10.0f;
                mc_fmt_float(raw, sizeof(raw), 0U, val, 1U);
                out_fg = kOrangeTgt;
            }
            break;
        }

        case 5: { // موقعیت X
            const float pos = mc_position_get(kAxisX);
            mc_fmt_float(raw, sizeof(raw), 0U, pos, 2U);
            out_fg = kColorWhite;
            break;
        }

        case 6: { // موقعیت Y
            const float pos = mc_position_get(kAxisY);
            mc_fmt_float(raw, sizeof(raw), 0U, pos, 2U);
            out_fg = kColorWhite;
            break;
        }

        case 7: { // موقعیت Z
            const float pos = mc_position_get(kAxisZ);
            mc_fmt_float(raw, sizeof(raw), 0U, pos, 2U);
            out_fg = kColorWhite;
            break;
        }

        case 8: { // موقعیت E
            const float pos = mc_position_get(kAxisE);
            mc_fmt_float(raw, sizeof(raw), 0U, pos, 2U);
            out_fg = kColorWhite;
            break;
        }

        default:
            break;
    }

    pad_and_copy(out_buf, cap, raw, kFields[idx].width);
}

// نقاشی یک مرحله از قاب ثابت (۱۰ مرحله)
void draw_static_step(uint8_t step) {
    switch (step) {
        case 0:
            mc_display_fill_screen(kColorBlack);
            break;
        case 1:
            mc_gfx_fill_rect(0U, 0U, 480U, 32U, kTopBarBg);
            break;
        case 2:
            mc_gfx_text(8U, 8U, "MakerCore", kColorWhite, kTopBarBg, 2U);
            break;
        case 3:
            mc_gfx_text(8U, 52U, "NOZZLE", kLabelGray, kColorBlack, 2U);
            break;
        case 4:
            mc_gfx_text(252U, 48U, "/", kColorWhite, kColorBlack, 3U);
            mc_gfx_text(372U, 48U, "C", kColorWhite, kColorBlack, 3U);
            break;
        case 5:
            mc_gfx_text(8U, 100U, "BED", kLabelGray, kColorBlack, 2U);
            break;
        case 6:
            mc_gfx_text(252U, 96U, "/", kColorWhite, kColorBlack, 3U);
            mc_gfx_text(372U, 96U, "C", kColorWhite, kColorBlack, 3U);
            break;
        case 7:
            mc_gfx_hline(8U, 144U, 464U, kDividerGray);
            break;
        case 8:
            mc_gfx_text(8U, 160U, "X", kLabelGray, kColorBlack, 2U);
            mc_gfx_text(248U, 160U, "Y", kLabelGray, kColorBlack, 2U);
            mc_gfx_text(8U, 208U, "Z", kLabelGray, kColorBlack, 2U);
            mc_gfx_text(248U, 208U, "E", kLabelGray, kColorBlack, 2U);
            break;
        case 9:
            mc_gfx_text(8U, 304U, MC_FIRMWARE_NAME " " MC_VERSION_STRING, kDividerGray, kColorBlack, 1U);
            break;
        default:
            break;
    }
    mc_dbg_295_ui_draw_cnt = mc_dbg_295_ui_draw_cnt + 1U;
}

// بررسی و نقاشی یک فیلد متغیر در صورت تغییر
void update_one_field() {
    char new_text[16];
    uint16_t new_fg = kColorWhite;
    format_field(g_current_field_idx, new_text, sizeof(new_text), new_fg);

    FieldCache& cache = g_field_cache[g_current_field_idx];
    const FieldSpec& spec = kFields[g_current_field_idx];

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

    g_current_field_idx = static_cast<uint8_t>((g_current_field_idx + 1U) % 9U);
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

    for (uint8_t i = 0U; i < 9U; ++i) {
        g_field_cache[i].text[0] = '\0';
        g_field_cache[i].fg_color = 0xFFFFU;
        g_field_cache[i].dirty = true;
    }

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
            if (g_static_frame_step < 10U) {
                draw_static_step(g_static_frame_step);
                g_static_frame_step++;
                if (g_static_frame_step >= 10U) {
                    for (uint8_t i = 0U; i < 9U; ++i) {
                        g_field_cache[i].dirty = true;
                        g_field_cache[i].text[0] = '\0';
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
