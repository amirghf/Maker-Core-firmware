// ذخیره‌سازی سخت‌دار تنظیمات روی EEPROM با CRC32 و دو اسلات A و B (بند ۷.۲) + فرمان‌های آزمایشی.
#include "settings_store.h"
#include "eeprom_at24.h"
#include "crc32.h"
#include "hal_time.h"

// ---------------- متغیرهای debug (Live Expressions در CubeIDE) ----------------
extern "C" {
volatile uint32_t mc_dbg_266_st_cmd           = 0;  // نوشتنی: فرمان آزمایشی (0 بیکار؛ 1 ذخیره داده آزمایشی؛ 2 بازیابی؛ 3 ذخیره با CRC خراب (شبیه‌سازی قطع برق)؛ 4 خراب‌کردن یک بایت داده اسلات A؛ 5 خراب‌کردن یک بایت داده اسلات B؛ 6 پاک‌کردن سرآیند هر دو اسلات). بعد از اجرا خودش 0 می‌شود
volatile uint32_t mc_dbg_267_st_seed          = 0;  // نوشتنی: عدد شروع الگوی داده آزمایشی (فقط 8 بیت پایین؛ بایت شماره i برابر 0x40 + i + seed)
volatile uint32_t mc_dbg_268_st_state         = 0;  // خواندنی: وضعیت (0 بیکار، 1 در جریان، 2 موفق، 3 خطا)
volatile uint32_t mc_dbg_269_st_result        = 0;  // خواندنی: کد نتیجه (0 هیچ، 1 موفق، 2 خطای EEPROM یا زمان، 3 هیچ اسلات سالم نبود، 4 خطای verify، 5 درخواست نامعتبر)
volatile uint32_t mc_dbg_270_st_crc_selftest  = 0;  // خواندنی: 1 = خودآزمایی CRC32 (داده 123456789) درست بود
volatile uint32_t mc_dbg_271_st_slot_a_status = 0;  // خواندنی: وضعیت اسلات A در آخرین بررسی (0 بررسی نشده، 1 سالم، 2 خراب)
volatile uint32_t mc_dbg_272_st_slot_b_status = 0;  // خواندنی: وضعیت اسلات B در آخرین بررسی (0 بررسی نشده، 1 سالم، 2 خراب)
volatile uint32_t mc_dbg_273_st_slot_a_seq    = 0;  // خواندنی: شماره دنباله اسلات A در آخرین بررسی (0 اگر خراب)
volatile uint32_t mc_dbg_274_st_slot_b_seq    = 0;  // خواندنی: شماره دنباله اسلات B در آخرین بررسی (0 اگر خراب)
volatile uint32_t mc_dbg_275_st_active_slot   = 0;  // خواندنی: اسلات انتخاب‌شده در آخرین بازیابی (0 هیچ، 1 اسلات A، 2 اسلات B)
volatile uint32_t mc_dbg_276_st_loaded_len    = 0;  // خواندنی: طول داده آخرین بازیابی موفق (بایت)
volatile uint32_t mc_dbg_277_st_loaded_first  = 0;  // خواندنی: اولین بایت داده آخرین بازیابی موفق
volatile uint32_t mc_dbg_278_st_loaded_last   = 0;  // خواندنی: آخرین بایت داده آخرین بازیابی موفق
volatile uint32_t mc_dbg_279_st_loaded_seq    = 0;  // خواندنی: شماره دنباله اسلات آخرین بازیابی موفق
volatile uint32_t mc_dbg_280_st_save_cnt      = 0;  // خواندنی: تعداد ذخیره‌های موفق (فقط فرمان 1 و نه فرمان 3)
volatile uint32_t mc_dbg_281_st_last_save_slot = 0; // خواندنی: اسلاتی که آخرین ذخیره در آن نوشت (0 هیچ، 1 اسلات A، 2 اسلات B)
}

namespace mc {
namespace {

constexpr uint16_t kSlotBytes     = MC_SETTINGS_SLOT_BYTES;
constexpr uint16_t kHeaderBytes   = MC_SETTINGS_HEADER_BYTES;
constexpr uint16_t kPayloadMax    = static_cast<uint16_t>(kSlotBytes - kHeaderBytes);
constexpr uint32_t kMagic         = MC_SETTINGS_MAGIC;
constexpr uint16_t kVersion       = MC_SETTINGS_FORMAT_VERSION;
constexpr uint32_t kStepTimeoutMs = MC_SETTINGS_STEP_TIMEOUT_MS;
constexpr uint16_t kTestLen       = MC_SETTINGS_TEST_PAYLOAD_LEN;

// جای فیلدها در سرآیند (کم‌ارزش‌ترین بایت اول)
constexpr uint16_t kOffMagic   = 0U;   // 4 بایت
constexpr uint16_t kOffVersion = 4U;   // 2 بایت
constexpr uint16_t kOffLen     = 6U;   // 2 بایت: طول داده
constexpr uint16_t kOffSeq     = 8U;   // 4 بایت: شماره دنباله
constexpr uint16_t kOffCrc     = 12U;  // 4 بایت: CRC32 (روی سرآیند با CRC صفرشده + داده)

static_assert(MC_SETTINGS_HEADER_BYTES == 16, "سرآیند باید 16 بایت باشد");
static_assert(MC_SETTINGS_TEST_PAYLOAD_LEN <= (MC_SETTINGS_SLOT_BYTES - MC_SETTINGS_HEADER_BYTES), "داده آزمایشی در اسلات جا نمی‌شود");
static_assert(MC_SETTINGS_SLOT_B_ADDR >= (MC_SETTINGS_SLOT_A_ADDR + MC_SETTINGS_SLOT_BYTES), "اسلات B با A هم‌پوشانی دارد");
static_assert((MC_SETTINGS_SLOT_B_ADDR + MC_SETTINGS_SLOT_BYTES) <= 4032, "اسلات‌ها با ناحیه تست EEPROM هم‌پوشانی دارند");

enum class Mode : uint8_t { Load = 0, Save = 1, SaveBroken = 2, FlipA = 3, FlipB = 4, EraseBoth = 5 };
enum class Step : uint8_t { Idle = 0, ReadA = 1, ReadB = 2, WriteSlot = 3, VerifyRead = 4 };

struct Slot {
    uint8_t  img[kSlotBytes];   // محتوای خوانده‌شده از EEPROM
    bool     valid;             // سالم بودن (magic، نسخه، طول، CRC)
    uint16_t len;               // طول داده (فقط اگر سالم)
    uint32_t seq;               // شماره دنباله (فقط اگر سالم)
};

struct Ctl {
    SettingsState state;
    uint8_t   result;
    Mode      mode;
    Step      step;
    bool      job_started;     // کار EEPROM این مرحله شروع شده است
    uint32_t  t_step_ms;       // زمان شروع مرحله
    uint8_t   target;          // اسلات مقصد نوشتن (0 = A، 1 = B)
    uint16_t  waddr;           // آدرس نوشتن
    uint16_t  wlen;            // طول نوشتن
    uint16_t  save_len;        // طول داده ذخیره
    uint32_t  new_seq;         // شماره دنباله ذخیره جدید
    uint8_t*  load_dst;        // مقصد بازیابی
    uint16_t  load_max;        // اندازه بافر مقصد
    uint16_t  loaded_len;
    uint32_t  loaded_seq;
    uint8_t   loaded_first;
    uint8_t   loaded_last;
    uint8_t   active;          // 0 هیچ، 1 A، 2 B
    uint8_t   last_save_slot;  // 0 هیچ، 1 A، 2 B
    uint32_t  save_cnt;
    bool      crc_ok;
    uint8_t   slot_st[2];      // 0 بررسی نشده، 1 سالم، 2 خراب
    uint32_t  slot_seq[2];
};

Ctl     g;
Slot    g_slot[2];
uint8_t g_wbuf[kSlotBytes];     // تصویر نوشتن
uint8_t g_vbuf[kSlotBytes];     // خواندن برای verify
uint8_t g_pay[kPayloadMax];     // کپی داده ذخیره
uint8_t g_test_pay[kPayloadMax];   // داده آزمایشی
uint8_t g_test_load[kPayloadMax];  // مقصد بازیابی آزمایشی

// ---------------- کمکی‌ها ----------------
void put16(uint8_t* p, uint16_t v) { p[0] = static_cast<uint8_t>(v & 0xFFU); p[1] = static_cast<uint8_t>(v >> 8U); }
void put32(uint8_t* p, uint32_t v) {
    p[0] = static_cast<uint8_t>(v & 0xFFU);
    p[1] = static_cast<uint8_t>((v >> 8U) & 0xFFU);
    p[2] = static_cast<uint8_t>((v >> 16U) & 0xFFU);
    p[3] = static_cast<uint8_t>((v >> 24U) & 0xFFU);
}
uint16_t get16(const uint8_t* p) { return static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8U)); }
uint32_t get32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8U) |
           (static_cast<uint32_t>(p[2]) << 16U) | (static_cast<uint32_t>(p[3]) << 24U);
}

uint16_t slot_addr(uint8_t idx) { return (idx == 0U) ? kSettingsSlotAAddr : kSettingsSlotBAddr; }

// سرآیند و داده را بررسی می‌کند و s.valid، s.len و s.seq را پر می‌کند.
bool check_slot(Slot& s) {
    s.valid = false; s.len = 0U; s.seq = 0U;
    if (get32(&s.img[kOffMagic]) != kMagic) { return false; }
    if (get16(&s.img[kOffVersion]) != kVersion) { return false; }
    const uint16_t len = get16(&s.img[kOffLen]);
    if ((len == 0U) || (len > kPayloadMax)) { return false; }
    const uint32_t stored = get32(&s.img[kOffCrc]);
    put32(&s.img[kOffCrc], 0U);
    const uint32_t calc = mc_crc32(s.img, static_cast<uint16_t>(kHeaderBytes + len));
    put32(&s.img[kOffCrc], stored);   // مقدار اصلی برمی‌گردد
    if (calc != stored) { return false; }
    s.valid = true; s.len = len; s.seq = get32(&s.img[kOffSeq]);
    return true;
}

// سالم‌ترین اسلات: 0 = A، 1 = B، -1 = هیچ‌کدام. مقایسه seq با چرخش 32 بیتی امن است.
int32_t best_slot() {
    const bool a = g_slot[0].valid;
    const bool b = g_slot[1].valid;
    if (a && b) {
        return (static_cast<int32_t>(g_slot[0].seq - g_slot[1].seq) > 0) ? 0 : 1;
    }
    if (a) { return 0; }
    if (b) { return 1; }
    return -1;
}

void build_image(uint8_t* img, const uint8_t* pay, uint16_t len, uint32_t seq) {
    put32(&img[kOffMagic], kMagic);
    put16(&img[kOffVersion], kVersion);
    put16(&img[kOffLen], len);
    put32(&img[kOffSeq], seq);
    put32(&img[kOffCrc], 0U);
    for (uint16_t i = 0U; i < len; i++) { img[kHeaderBytes + i] = pay[i]; }
    const uint32_t crc = mc_crc32(img, static_cast<uint16_t>(kHeaderBytes + len));
    put32(&img[kOffCrc], crc);
}

void fail(uint8_t res) {
    g.state = SettingsState::Error;
    g.result = res;
    g.step = Step::Idle;
    g.job_started = false;
}

void finish_ok() {
    g.state = SettingsState::Done;
    g.result = kSetResOk;
    g.step = Step::Idle;
    g.job_started = false;
}

// شروع یک کار. EraseBoth بدون خواندن مستقیم به مرحله نوشتن می‌رود.
bool begin(Mode m) {
    if (g.state == SettingsState::Busy) { return false; }
    if (!mc_eeprom_present()) { fail(kSetResEepromFail); return false; }
    g.mode = m;
    g.job_started = false;
    g.t_step_ms = mc_millis();
    g.state = SettingsState::Busy;
    g.result = kSetResNone;
    g.step = (m == Mode::EraseBoth) ? Step::WriteSlot : Step::ReadA;
    return true;
}

bool start_save(const uint8_t* payload, uint16_t len, Mode m) {
    if ((payload == nullptr) || (len == 0U) || (len > kPayloadMax)) { fail(kSetResRefused); return false; }
    if (g.state == SettingsState::Busy) { return false; }
    for (uint16_t i = 0U; i < len; i++) { g_pay[i] = payload[i]; }
    g.save_len = len;
    return begin(m);
}

void update_slot_debug_state() {
    for (uint8_t i = 0U; i < 2U; i++) {
        g.slot_st[i]  = g_slot[i].valid ? 1U : 2U;
        g.slot_seq[i] = g_slot[i].valid ? g_slot[i].seq : 0U;
    }
}

// بعد از خواندن هر دو اسلات: بسته به نوع کار ادامه می‌دهد.
void after_scan() {
    update_slot_debug_state();
    const int32_t best = best_slot();
    switch (g.mode) {
        case Mode::Load: {
            if (best < 0) { g.active = 0U; fail(kSetResNoValid); return; }
            const Slot& s = g_slot[best];
            if ((g.load_dst == nullptr) || (s.len > g.load_max)) { fail(kSetResRefused); return; }
            for (uint16_t i = 0U; i < s.len; i++) { g.load_dst[i] = s.img[kHeaderBytes + i]; }
            g.loaded_len   = s.len;
            g.loaded_seq   = s.seq;
            g.loaded_first = g.load_dst[0];
            g.loaded_last  = g.load_dst[s.len - 1U];
            g.active       = static_cast<uint8_t>(best + 1);
            finish_ok();
            return;
        }
        case Mode::Save:
        case Mode::SaveBroken: {
            g.target  = (best == 0) ? 1U : 0U;               // همیشه اسلات قدیمی‌تر یا خراب (اگر هیچ سالم نبود، A)
            g.new_seq = (best < 0) ? 1U : (g_slot[best].seq + 1U);
            build_image(g_wbuf, g_pay, g.save_len, g.new_seq);
            if (g.mode == Mode::SaveBroken) {
                g_wbuf[kHeaderBytes] = static_cast<uint8_t>(g_wbuf[kHeaderBytes] ^ 0xFFU);   // CRC دیگر نمی‌خواند
            }
            g.waddr = slot_addr(g.target);
            g.wlen  = static_cast<uint16_t>(kHeaderBytes + g.save_len);
            g.step  = Step::WriteSlot;
            g.job_started = false;
            g.t_step_ms = mc_millis();
            return;
        }
        case Mode::FlipA:
        case Mode::FlipB: {
            g.target = (g.mode == Mode::FlipA) ? 0U : 1U;
            g_wbuf[0] = static_cast<uint8_t>(g_slot[g.target].img[kHeaderBytes] ^ 0xFFU);   // اولین بایت داده معکوس می‌شود
            g.waddr = static_cast<uint16_t>(slot_addr(g.target) + kHeaderBytes);
            g.wlen  = 1U;
            g.step  = Step::WriteSlot;
            g.job_started = false;
            g.t_step_ms = mc_millis();
            return;
        }
        default:
            fail(kSetResRefused);
            return;
    }
}

void poll_machine() {
    if (g.state != SettingsState::Busy) { return; }
    if (mc_time_elapsed_ms(g.t_step_ms, kStepTimeoutMs)) { fail(kSetResEepromFail); return; }
    switch (g.step) {
        case Step::ReadA:
        case Step::ReadB: {
            const uint8_t idx = (g.step == Step::ReadA) ? 0U : 1U;
            if (!g.job_started) {
                if (!mc_eeprom_read_start(slot_addr(idx), g_slot[idx].img, kSlotBytes)) { return; }   // EEPROM مشغول است؛ دور بعد
                g.job_started = true;
                return;
            }
            if (mc_eeprom_state() == EepromState::Busy) { return; }
            g.job_started = false;
            if (mc_eeprom_state() != EepromState::Done) { fail(kSetResEepromFail); return; }
            (void)check_slot(g_slot[idx]);
            g.t_step_ms = mc_millis();
            if (g.step == Step::ReadA) { g.step = Step::ReadB; } else { after_scan(); }
            return;
        }
        case Step::WriteSlot: {
            if (!g.job_started) {
                if (!mc_eeprom_write_start(g.waddr, g_wbuf, g.wlen)) { return; }
                g.job_started = true;
                return;
            }
            if (mc_eeprom_state() == EepromState::Busy) { return; }
            g.job_started = false;
            if (mc_eeprom_state() != EepromState::Done) { fail(kSetResEepromFail); return; }
            g.t_step_ms = mc_millis();
            if ((g.mode == Mode::EraseBoth) && (g.target == 0U)) {   // اسلات A پاک شد؛ حالا B
                g.target = 1U;
                g.waddr  = slot_addr(1U);
                return;                                              // در همین مرحله می‌ماند
            }
            if (g.mode == Mode::Save) { g.step = Step::VerifyRead; return; }
            if (g.mode == Mode::SaveBroken) { g.last_save_slot = static_cast<uint8_t>(g.target + 1U); }
            finish_ok();
            return;
        }
        case Step::VerifyRead: {
            if (!g.job_started) {
                if (!mc_eeprom_read_start(g.waddr, g_vbuf, g.wlen)) { return; }
                g.job_started = true;
                return;
            }
            if (mc_eeprom_state() == EepromState::Busy) { return; }
            g.job_started = false;
            if (mc_eeprom_state() != EepromState::Done) { fail(kSetResEepromFail); return; }
            for (uint16_t i = 0U; i < g.wlen; i++) {
                if (g_vbuf[i] != g_wbuf[i]) { fail(kSetResVerifyFail); return; }
            }
            g.save_cnt = g.save_cnt + 1U;
            g.last_save_slot = static_cast<uint8_t>(g.target + 1U);
            finish_ok();
            return;
        }
        default:
            fail(kSetResEepromFail);
            return;
    }
}

// ---------------- فرمان‌های آزمایشی ----------------
void fill_test_payload(uint8_t seed) {
    for (uint16_t i = 0U; i < kTestLen; i++) { g_test_pay[i] = static_cast<uint8_t>(0x40U + i + seed); }
}

void test_poll() {
    const uint32_t cmd = mc_dbg_266_st_cmd;
    if (cmd == 0U) { return; }
    mc_dbg_266_st_cmd = 0U;   // هر فرمان فقط یک‌بار اجرا می‌شود
    const uint8_t seed = static_cast<uint8_t>(mc_dbg_267_st_seed & 0xFFU);
    switch (cmd) {
        case 1U:   // ذخیره داده آزمایشی (نوشتن + verify)
            fill_test_payload(seed);
            (void)start_save(g_test_pay, kTestLen, Mode::Save);
            break;
        case 2U:   // بازیابی از سالم‌ترین اسلات
            g.load_dst = g_test_load;
            g.load_max = kPayloadMax;
            (void)begin(Mode::Load);
            break;
        case 3U:   // ذخیره با CRC خراب عمدی (شبیه‌سازی قطع برق وسط نوشتن؛ بدون verify)
            fill_test_payload(seed);
            (void)start_save(g_test_pay, kTestLen, Mode::SaveBroken);
            break;
        case 4U:   // خراب‌کردن اولین بایت داده اسلات A
            (void)begin(Mode::FlipA);
            break;
        case 5U:   // خراب‌کردن اولین بایت داده اسلات B
            (void)begin(Mode::FlipB);
            break;
        case 6U:   // پاک‌کردن سرآیند (16 بایت 0xFF) هر دو اسلات
            for (uint16_t i = 0U; i < kHeaderBytes; i++) { g_wbuf[i] = 0xFFU; }
            g.target = 0U;
            g.waddr  = slot_addr(0U);
            g.wlen   = kHeaderBytes;
            (void)begin(Mode::EraseBoth);
            break;
        default:
            break;
    }
}

void refresh_debug() {
    mc_dbg_268_st_state          = static_cast<uint32_t>(g.state);
    mc_dbg_269_st_result         = static_cast<uint32_t>(g.result);
    mc_dbg_270_st_crc_selftest   = g.crc_ok ? 1U : 0U;
    mc_dbg_271_st_slot_a_status  = g.slot_st[0];
    mc_dbg_272_st_slot_b_status  = g.slot_st[1];
    mc_dbg_273_st_slot_a_seq     = g.slot_seq[0];
    mc_dbg_274_st_slot_b_seq     = g.slot_seq[1];
    mc_dbg_275_st_active_slot    = g.active;
    mc_dbg_276_st_loaded_len     = g.loaded_len;
    mc_dbg_277_st_loaded_first   = g.loaded_first;
    mc_dbg_278_st_loaded_last    = g.loaded_last;
    mc_dbg_279_st_loaded_seq     = g.loaded_seq;
    mc_dbg_280_st_save_cnt       = g.save_cnt;
    mc_dbg_281_st_last_save_slot = g.last_save_slot;
}

}  // namespace

void mc_settings_init() {
    static const uint8_t kVec[9] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    g.crc_ok = (mc_crc32(kVec, 9U) == 0xCBF43926U);   // فقط محاسبه؛ به EEPROM دست نمی‌زند
    refresh_debug();
}

void mc_settings_poll() {
    test_poll();
    poll_machine();
    refresh_debug();
}

bool mc_settings_save_start(const uint8_t* payload, uint16_t len) {
    return start_save(payload, len, Mode::Save);
}

bool mc_settings_load_start(uint8_t* out, uint16_t max_len) {
    if ((out == nullptr) || (max_len == 0U)) { fail(kSetResRefused); return false; }
    if (g.state == SettingsState::Busy) { return false; }
    g.load_dst = out;
    g.load_max = max_len;
    return begin(Mode::Load);
}

SettingsState mc_settings_state()  { return g.state; }
uint8_t       mc_settings_result() { return g.result; }
uint16_t      mc_settings_loaded_len() { return g.loaded_len; }

}  // namespace mc
