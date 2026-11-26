// صفحه پیام‌های Error، Warning و Info (بند ۱۰.۱).
// هیچ هدر ST اینجا include نمی‌شود.
#include "ui_msg.h"
#include "ui_home.h"
#include "ui_colors.h"
#include "ui_touch.h"
#include "../core/gfx.h"
#include "../core/encoder.h"
#include "../hal/hal_display.h"
#include "../config/mc_config.h"

extern "C" {
extern volatile uint32_t mc_dbg_294_ui_state;   // خواندنی (تعریف در ui_home.cpp): وضعیت UI؛ 4 = کار عادی (بند ۱۰.۱)

volatile uint32_t mc_dbg_464_msg_force          = 0;  // نوشتنی: 0 = بی‌اثر. 1 = Error نمونه، 2 = Warning نمونه، 3 = Info نمونه.
volatile uint32_t mc_dbg_465_msg_sample         = 0;  // نوشتنی: شماره نمونه متن 0 تا 3.
volatile uint32_t mc_dbg_466_msg_shown          = 0;  // خواندنی: 1 = صفحه پیام الان نمایش داده می‌شود، 0 = نه.
volatile uint32_t mc_dbg_467_msg_type           = 0;  // خواندنی: نوع پیام فعلی: 0 = هیچ، 1 = Error، 2 = Warning، 3 = Info.
volatile uint32_t mc_dbg_468_msg_show_cnt       = 0;  // خواندنی: تعداد دفعاتی که پیام نمایش داده شد.
volatile uint32_t mc_dbg_469_msg_close_cnt      = 0;  // خواندنی: تعداد دفعاتی که پیام با OK یا انکودر بسته شد.
volatile uint32_t mc_dbg_470_msg_close_src      = 0;  // خواندنی: منبع آخرین بستن: 0 = هنوز، 1 = دکمه انکودر، 2 = لمس دکمه OK.
volatile uint32_t mc_dbg_471_msg_refused        = 0;  // خواندنی: تعداد دفعاتی که درخواست نمایش پیام رد شد.
volatile uint32_t mc_dbg_472_msg_refuse_reason  = 0;  // خواندنی: علت آخرین رد شدن: 0 = هنوز، 1 = صفحه فعال 0 نبود یا UI آماده نبود، 2 = نوع/نمونه نامعتبر.
volatile uint32_t mc_dbg_494_msg_queue_depth    = 0;  // خواندنی: تعداد پیام‌های در انتظار و باز (بند ۱۰.۱۳)
volatile uint32_t mc_dbg_495_msg_queue_drop_cnt = 0;  // خواندنی: تعداد پیام‌هایی که چون صف پر بود دور ریخته شدند (بند ۱۰.۱۳)
}

namespace mc {
namespace {

constexpr uint16_t kSelBg     = mc_rgb565(24, 38, 64);
constexpr uint16_t kErrBar    = mc_rgb565(150, 25, 25);
constexpr uint16_t kErrAcc    = kAccBed;                       // mc_rgb565(240, 70, 70)
constexpr uint16_t kWarnBar   = mc_rgb565(120, 80, 0);
constexpr uint16_t kWarnAcc   = mc_rgb565(255, 170, 0);
constexpr uint16_t kInfoBar   = kTopBarBg;
constexpr uint16_t kInfoAcc   = kAccentLine;

struct SampleMsg {
    const char* text;
    const char* detail;
};

static const SampleMsg kSamples[4] = {
    { "NOZZLE TEMPERATURE TOO HIGH", "CODE 5 - MAXTEMP EXTRUDER" },
    { "NOZZLE DID NOT HEAT UP IN TIME. CHECK HEATER AND THERMISTOR WIRES.", "CODE 8 - WATCH HEATING" },
    { "THIS IS A VERY LONG TEST MESSAGE USED ONLY TO CHECK THAT THE TEXT WRAPS ON THREE LINES AND THE REST IS CUT WITH THREE DOTS AT THE END OF THE LAST LINE", "DETAIL LINE USED TO TEST THE SMALL GRAY TEXT UNDER THE DIVIDER" },
    { "FILAMENT RUNOUT", "" }
};

struct QueuedMsg {
    MsgType type = MsgType::None;
    char text[MC_UI_MSG_TEXT_MAX + 1U];
    char detail[MC_UI_MSG_DETAIL_MAX + 1U];
};

static QueuedMsg g_queue[MC_UI_MSG_QUEUE_LEN];
static uint8_t   g_queue_head  = 0U;
static uint8_t   g_queue_count = 0U;

static MsgType g_msg_type = MsgType::None;
static char    g_text_buf[MC_UI_MSG_TEXT_MAX + 1U];
static char    g_detail_buf[MC_UI_MSG_DETAIL_MAX + 1U];
static bool    g_has_detail = false;
static char    g_lines[MC_UI_MSG_MAX_LINES][MC_UI_MSG_LINE_CHARS + 1U];
static uint8_t g_line_count = 0U;
static uint8_t g_msg_step   = 255U;
static bool    g_pending    = false;

// محاسبه جذر صحیح (بدون float)
static int16_t isqrt(int32_t n) {
    if (n <= 0) { return 0; }
    int32_t x = 0;
    while ((x + 1) * (x + 1) <= n) {
        x++;
    }
    return static_cast<int16_t>(x);
}

// رسم دیسک پر با قطر 2*r+1
static void draw_disc(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
    for (int16_t dy = -r; dy <= r; ++dy) {
        const int16_t dx = isqrt(static_cast<int32_t>(r * r - dy * dy));
        mc_gfx_hline(static_cast<uint16_t>(cx - dx),
                     static_cast<uint16_t>(cy + dy),
                     static_cast<uint16_t>(2 * dx + 1),
                     color);
    }
}

// رسم خط ضخیم با الگوریتم صحیح Bresenham (بدون float)
static void draw_thick_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t half, uint16_t color) {
    int16_t dx = (x0 < x1) ? (x1 - x0) : (x0 - x1);
    int16_t dy = (y0 < y1) ? (y1 - y0) : (y0 - y1);
    const int16_t sx = (x0 < x1) ? 1 : -1;
    const int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;
    const uint16_t sz = static_cast<uint16_t>(2 * half + 1);

    while (true) {
        mc_gfx_fill_rect(static_cast<uint16_t>(x0 - half),
                         static_cast<uint16_t>(y0 - half),
                         sz, sz, color);
        if (x0 == x1 && y0 == y1) { break; }
        const int16_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// شکستن متن اصلی به سطرها با کلمه-شکست و برش سه نقطه
static void format_text_lines() {
    g_lines[0][0] = '\0';
    g_lines[1][0] = '\0';
    g_lines[2][0] = '\0';
    g_line_count = 0U;

    const char* p = g_text_buf;
    if (*p == '\0') { return; }

    for (uint8_t line_idx = 0U; line_idx < MC_UI_MSG_MAX_LINES; ++line_idx) {
        while (*p == ' ') { ++p; }
        if (*p == '\0') { break; }

        uint16_t len = 0U;
        int16_t last_space_idx = -1;

        const char* temp = p;
        while ((*temp != '\0') && (len < MC_UI_MSG_LINE_CHARS)) {
            if (*temp == ' ') {
                last_space_idx = static_cast<int16_t>(len);
            }
            ++len;
            ++temp;
        }

        uint16_t take_chars = len;
        if ((*temp != '\0') && (last_space_idx > 0)) {
            take_chars = static_cast<uint16_t>(last_space_idx);
        }

        for (uint16_t i = 0U; i < take_chars; ++i) {
            g_lines[line_idx][i] = *p;
            ++p;
        }
        g_lines[line_idx][take_chars] = '\0';
        g_line_count = static_cast<uint8_t>(line_idx + 1U);

        if (*p == ' ') { ++p; }

        if ((line_idx == (MC_UI_MSG_MAX_LINES - 1U)) && (*p != '\0')) {
            const uint16_t l3_len = take_chars;
            if (l3_len >= 3U) {
                g_lines[2][l3_len - 3U] = '.';
                g_lines[2][l3_len - 2U] = '.';
                g_lines[2][l3_len - 1U] = '.';
                g_lines[2][l3_len]      = '\0';
            } else {
                g_lines[2][0] = '.';
                g_lines[2][1] = '.';
                g_lines[2][2] = '.';
                g_lines[2][3] = '\0';
            }
            break;
        }
    }
}

static void draw_step_0() {
    uint16_t bar_col = kTopBarBg;
    uint16_t acc_col = kAccentLine;
    const char* title_txt = "INFO";

    if (g_msg_type == MsgType::Error) {
        bar_col = kErrBar;
        acc_col = kErrAcc;
        title_txt = "ERROR";
    } else if (g_msg_type == MsgType::Warning) {
        bar_col = kWarnBar;
        acc_col = kWarnAcc;
        title_txt = "WARNING";
    }

    mc_display_fill_screen(kScreenBg);
    mc_gfx_fill_rect(0U, 0U, 480U, 36U, bar_col);
    mc_gfx_fill_rect(0U, 36U, 480U, 2U, acc_col);
    const uint16_t tw = mc_gfx_text_width(title_txt, 2U);
    const uint16_t tx = static_cast<uint16_t>((480U - tw) / 2U);
    mc_gfx_text(tx, 10U, title_txt, kColorWhite, bar_col, 2U);
}

static void draw_step_1() {
    uint16_t acc_col = kAccentLine;
    if (g_msg_type == MsgType::Error) {
        acc_col = kErrAcc;
    } else if (g_msg_type == MsgType::Warning) {
        acc_col = kWarnAcc;
    }

    mc_gfx_fill_rect(14U, 50U, 452U, 172U, kCardBg);
    mc_gfx_rect(14U, 50U, 452U, 172U, acc_col);
    mc_gfx_fill_rect(14U, 50U, 4U, 172U, acc_col);
}

static void draw_step_2() {
    uint16_t acc_col = kAccentLine;
    if (g_msg_type == MsgType::Error) {
        acc_col = kErrAcc;
    } else if (g_msg_type == MsgType::Warning) {
        acc_col = kWarnAcc;
    }

    if (g_msg_type == MsgType::Error) {
        draw_disc(62, 98, 30, acc_col);
        draw_thick_line(49, 85, 75, 111, 2, kColorWhite);
        draw_thick_line(75, 85, 49, 111, 2, kColorWhite);
    } else if (g_msg_type == MsgType::Warning) {
        for (int16_t y = 68; y <= 126; ++y) {
            const int16_t half_w = static_cast<int16_t>(((y - 68) * 30) / 58);
            mc_gfx_hline(static_cast<uint16_t>(62 - half_w),
                         static_cast<uint16_t>(y),
                         static_cast<uint16_t>(2 * half_w + 1),
                         acc_col);
        }
        mc_gfx_fill_rect(60U, 90U, 5U, 22U, kColorWhite);
        mc_gfx_fill_rect(60U, 116U, 5U, 5U, kColorWhite);
    } else {
        draw_disc(62, 98, 30, acc_col);
        mc_gfx_fill_rect(59U, 82U, 7U, 7U, kColorWhite);
        mc_gfx_fill_rect(59U, 94U, 7U, 24U, kColorWhite);
    }
}

static void draw_step_3() {
    uint16_t start_y = 66U;
    if (!g_has_detail) {
        const uint16_t block_h = (g_line_count > 0U) ? static_cast<uint16_t>(g_line_count * 24U - 8U) : 16U;
        start_y = static_cast<uint16_t>(62U + (152U - block_h) / 2U);
    }

    for (uint8_t i = 0U; i < g_line_count; ++i) {
        const uint16_t line_y = static_cast<uint16_t>(start_y + i * 24U);
        mc_gfx_text(112U, line_y, g_lines[i], kColorWhite, kCardBg, 2U);
    }

    if (g_has_detail) {
        mc_gfx_hline(112U, 170U, 344U, kDividerGray);
        mc_gfx_text(112U, 182U, g_detail_buf, kLabelGray, kCardBg, 1U);
    }
}

static void draw_step_4() {
    uint16_t acc_col = kAccentLine;
    if (g_msg_type == MsgType::Error) {
        acc_col = kErrAcc;
    } else if (g_msg_type == MsgType::Warning) {
        acc_col = kWarnAcc;
    }

    constexpr uint16_t bx = 170U;
    constexpr uint16_t by = 240U;
    constexpr uint16_t bw = 140U;
    constexpr uint16_t bh = 48U;

    mc_gfx_fill_rect(bx, by, bw, bh, kSelBg);
    mc_gfx_rect(bx, by, bw, bh, acc_col);
    mc_gfx_rect(static_cast<uint16_t>(bx + 1U), static_cast<uint16_t>(by + 1U),
                static_cast<uint16_t>(bw - 2U), static_cast<uint16_t>(bh - 2U), acc_col);

    const uint16_t tw = mc_gfx_text_width("OK", 2U);
    const uint16_t tx = static_cast<uint16_t>(bx + (bw - tw) / 2U);
    const uint16_t ty = static_cast<uint16_t>(by + (bh - 16U) / 2U);
    mc_gfx_text(tx, ty, "OK", kColorWhite, kSelBg, 2U);

    const char* help_txt = "PRESS ENCODER OR TAP OK";
    const uint16_t hw = mc_gfx_text_width(help_txt, 1U);
    const uint16_t hx = static_cast<uint16_t>((480U - hw) / 2U);
    mc_gfx_text(hx, 300U, help_txt, kLabelGray, kScreenBg, 1U);
}

}  // namespace

void mc_ui_msg_init() {
    g_msg_type      = MsgType::None;
    g_text_buf[0]   = '\0';
    g_detail_buf[0] = '\0';
    g_has_detail    = false;
    g_line_count    = 0U;
    g_msg_step      = 255U;
    g_pending       = false;

    g_queue_head    = 0U;
    g_queue_count   = 0U;

    mc_dbg_464_msg_force         = 0U;
    mc_dbg_465_msg_sample        = 0U;
    mc_dbg_466_msg_shown         = 0U;
    mc_dbg_467_msg_type          = 0U;
    mc_dbg_468_msg_show_cnt      = 0U;
    mc_dbg_469_msg_close_cnt     = 0U;
    mc_dbg_470_msg_close_src     = 0U;
    mc_dbg_471_msg_refused       = 0U;
    mc_dbg_472_msg_refuse_reason = 0U;
    mc_dbg_494_msg_queue_depth   = 0U;
    mc_dbg_495_msg_queue_drop_cnt = 0U;
}

void mc_ui_msg_show(MsgType type, const char* text, const char* detail) {
    if (type == MsgType::None) { return; }

    char up_text[MC_UI_MSG_TEXT_MAX + 1U];
    char up_detail[MC_UI_MSG_DETAIL_MAX + 1U];

    uint16_t i = 0U;
    if (text != nullptr) {
        while ((text[i] != '\0') && (i < MC_UI_MSG_TEXT_MAX)) {
            char ch = text[i];
            if ((ch >= 'a') && (ch <= 'z')) { ch = static_cast<char>(ch - 'a' + 'A'); }
            up_text[i] = ch;
            ++i;
        }
    }
    up_text[i] = '\0';

    i = 0U;
    if (detail != nullptr) {
        while ((detail[i] != '\0') && (i < MC_UI_MSG_DETAIL_MAX)) {
            char ch = detail[i];
            if ((ch >= 'a') && (ch <= 'z')) { ch = static_cast<char>(ch - 'a' + 'A'); }
            up_detail[i] = ch;
            ++i;
        }
    }
    up_detail[i] = '\0';

    // سرریز صف: اگر صف پر است
    if (g_queue_count >= MC_UI_MSG_QUEUE_LEN) {
        if (type == MsgType::Error) {
            // پیام جدید Error است: قدیمی‌ترین Warning یا Info حذف می‌شود
            int16_t drop_idx = -1;
            for (uint8_t k = 0U; k < g_queue_count; ++k) {
                const uint8_t idx = static_cast<uint8_t>((g_queue_head + k) % MC_UI_MSG_QUEUE_LEN);
                if (g_queue[idx].type != MsgType::Error) {
                    drop_idx = static_cast<int16_t>(idx);
                    break;
                }
            }
            if (drop_idx >= 0) {
                uint8_t curr = static_cast<uint8_t>(drop_idx);
                const uint8_t last_idx = static_cast<uint8_t>((g_queue_head + g_queue_count - 1U) % MC_UI_MSG_QUEUE_LEN);
                while (curr != last_idx) {
                    const uint8_t nxt = static_cast<uint8_t>((curr + 1U) % MC_UI_MSG_QUEUE_LEN);
                    g_queue[curr] = g_queue[nxt];
                    curr = nxt;
                }
                g_queue_count--;
                mc_dbg_495_msg_queue_drop_cnt = mc_dbg_495_msg_queue_drop_cnt + 1U;
            } else {
                // همه پیام‌های صف Error هستند: پیام جدید دور ریخته می‌شود
                mc_dbg_495_msg_queue_drop_cnt = mc_dbg_495_msg_queue_drop_cnt + 1U;
                return;
            }
        } else {
            // پیام جدید Warning یا Info است: پیام جدید دور ریخته می‌شود
            mc_dbg_495_msg_queue_drop_cnt = mc_dbg_495_msg_queue_drop_cnt + 1U;
            return;
        }
    }

    // افزودن پیام به انتهای صف
    const uint8_t tail = static_cast<uint8_t>((g_queue_head + g_queue_count) % MC_UI_MSG_QUEUE_LEN);
    g_queue[tail].type = type;

    uint16_t k = 0U;
    while (up_text[k] != '\0') {
        g_queue[tail].text[k] = up_text[k];
        ++k;
    }
    g_queue[tail].text[k] = '\0';

    k = 0U;
    while (up_detail[k] != '\0') {
        g_queue[tail].detail[k] = up_detail[k];
        ++k;
    }
    g_queue[tail].detail[k] = '\0';

    g_queue_count++;
    mc_dbg_494_msg_queue_depth = g_queue_count;

    if (mc_dbg_466_msg_shown == 0U) {
        g_pending = true;
    }
}

bool mc_ui_msg_pending() {
    return g_pending;
}

void mc_ui_msg_open() {
    g_pending = false;
    if (g_queue_count == 0U) { return; }

    g_msg_type = g_queue[g_queue_head].type;

    uint16_t i = 0U;
    while (g_queue[g_queue_head].text[i] != '\0') {
        g_text_buf[i] = g_queue[g_queue_head].text[i];
        ++i;
    }
    g_text_buf[i] = '\0';

    i = 0U;
    while (g_queue[g_queue_head].detail[i] != '\0') {
        g_detail_buf[i] = g_queue[g_queue_head].detail[i];
        ++i;
    }
    g_detail_buf[i] = '\0';
    g_has_detail = (g_detail_buf[0] != '\0');

    format_text_lines();

    g_msg_step = 0U;
    mc_dbg_466_msg_shown = 1U;
    mc_dbg_467_msg_type  = static_cast<uint32_t>(g_msg_type);
    mc_dbg_468_msg_show_cnt = mc_dbg_468_msg_show_cnt + 1U;

    (void)mc_encoder_take_clicks();
    (void)mc_encoder_take_press();
    mc_ui_touch_reset();
}

MsgUiResult mc_ui_msg_poll() {
    if (g_msg_step < 255U) {
        switch (g_msg_step) {
            case 0:
                draw_step_0();
                g_msg_step = 1U;
                return MsgUiResult::Stay;
            case 1:
                draw_step_1();
                g_msg_step = 2U;
                return MsgUiResult::Stay;
            case 2:
                draw_step_2();
                g_msg_step = 3U;
                return MsgUiResult::Stay;
            case 3:
                draw_step_3();
                g_msg_step = 4U;
                return MsgUiResult::Stay;
            case 4:
                draw_step_4();
                g_msg_step = 255U;
                return MsgUiResult::Stay;
            default:
                g_msg_step = 255U;
                return MsgUiResult::Stay;
        }
    }

    bool closed = false;
    uint32_t close_src = 0U;

    // ۱) لمس (دکمه OK)
    {
        const TouchGesture tg = mc_ui_touch_poll();
        if (tg.evt == TouchEvt::Tap) {
            if (mc_ui_touch_in_rect(tg.x, tg.y, 170U, 240U, 140U, 48U)) {
                mc_ui_touch_accept(90U);
                closed = true;
                close_src = 2U;
            }
        }
    }

    // ۲) انکودر (فشردن دکمه)
    if (!closed && mc_encoder_take_press()) {
        closed = true;
        close_src = 1U;
    }

    if (closed) {
        mc_dbg_470_msg_close_src = close_src;
        mc_dbg_469_msg_close_cnt = mc_dbg_469_msg_close_cnt + 1U;

        if (g_queue_count > 0U) {
            g_queue_head = static_cast<uint8_t>((g_queue_head + 1U) % MC_UI_MSG_QUEUE_LEN);
            g_queue_count--;
        }
        mc_dbg_494_msg_queue_depth = g_queue_count;
        mc_dbg_466_msg_shown = 0U;
        mc_ui_touch_reset();

        if (g_queue_count > 0U) {
            g_pending = true;  // پیام بعدی صف دوباره pending می‌شود
        }
        return MsgUiResult::Closed;
    }

    // ۳) انکودر (چرخش در این صفحه کاری نمی‌کند)
    (void)mc_encoder_take_clicks();

    return MsgUiResult::Stay;
}

void mc_ui_msg_abort() {
    if (mc_dbg_466_msg_shown != 0U) {
        if (g_msg_type == MsgType::Error) {
            // پیام جاری Error است: در سر صف می‌ماند تا بعد از تعویض فیلامنت دوباره نشان داده شود
            g_pending = true;
        } else {
            // پیام جاری Warning یا Info است: از صف حذف می‌شود
            if (g_queue_count > 0U) {
                g_queue_head = static_cast<uint8_t>((g_queue_head + 1U) % MC_UI_MSG_QUEUE_LEN);
                g_queue_count--;
            }
            g_pending = (g_queue_count > 0U);
        }
    }
    g_msg_step = 255U;
    mc_dbg_466_msg_shown = 0U;
    mc_dbg_494_msg_queue_depth = g_queue_count;
}


MsgType mc_ui_msg_type() {
    return (mc_dbg_466_msg_shown != 0U) ? g_msg_type : MsgType::None;
}

void mc_ui_msg_test_poll() {
    if (mc_dbg_464_msg_force == 0U) { return; }

    const uint32_t force_val  = mc_dbg_464_msg_force;
    const uint32_t sample_val = mc_dbg_465_msg_sample;
    mc_dbg_464_msg_force = 0U;

    if ((force_val > 3U) || (sample_val > 3U)) {
        mc_dbg_471_msg_refused = mc_dbg_471_msg_refused + 1U;
        mc_dbg_472_msg_refuse_reason = 2U;
        return;
    }

    if ((mc_dbg_323_ui_page != 0U) || (mc_dbg_294_ui_state != 4U)) {
        mc_dbg_471_msg_refused = mc_dbg_471_msg_refused + 1U;
        mc_dbg_472_msg_refuse_reason = 1U;
        return;
    }

    const MsgType type = static_cast<MsgType>(force_val);
    const SampleMsg& smp = kSamples[sample_val];

    mc_ui_msg_show(type, smp.text, smp.detail);
}

}  // namespace mc
