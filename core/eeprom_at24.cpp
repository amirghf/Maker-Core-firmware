// درایور EEPROM نوع AT24C32D (بند ۷.۱) + فرمان‌های آزمایشی روی برد.
#include "eeprom_at24.h"
#include "hal_i2c.h"
#include "hal_time.h"

// ---------------- متغیرهای debug (Live Expressions در CubeIDE) ----------------
extern "C" {
volatile uint32_t mc_dbg_252_ee_cmd       = 0;  // نوشتنی: فرمان آزمایشی (0 بیکار؛ 1 ping؛ 2 نوشتن مرتب؛ 3 خواندن و مقایسه مرتب؛ 4 نوشتن نامرتب؛ 5 خواندن و مقایسه نامرتب؛ 6 نوشتن بیرون از حافظه (باید رد شود)؛ 7 فقط خواندن). بعد از اجرا خودش 0 می‌شود
volatile uint32_t mc_dbg_253_ee_seed      = 0;  // نوشتنی: عدد شروع الگوی تست (فقط 8 بیت پایین؛ بایت شماره i برابر 0x30 + i + seed)
volatile uint32_t mc_dbg_254_ee_state     = 0;  // خواندنی: وضعیت (0 بیکار، 1 در جریان، 2 موفق، 3 خطا)
volatile uint32_t mc_dbg_255_ee_result    = 0;  // خواندنی: کد نتیجه (0 هیچ، 1 موفق، 2 تراشه جواب نداد، 3 رد شد، 4 خطای نوشتن، 5 خطای خواندن، 6 تراشه بعد از نوشتن آماده نشد، 7 مقایسه نخواند)
volatile uint32_t mc_dbg_256_ee_found     = 0;  // خواندنی: 1 = تراشه در آخرین بررسی جواب داد
volatile uint32_t mc_dbg_257_ee_ok_cnt    = 0;  // خواندنی: تعداد کارهای موفق
volatile uint32_t mc_dbg_258_ee_err_cnt   = 0;  // خواندنی: تعداد کارهای ناموفق یا ردشده
volatile uint32_t mc_dbg_259_ee_refused   = 0;  // خواندنی: تعداد فرمان‌هایی که چون کار قبلی در جریان بود پذیرفته نشدند
volatile uint32_t mc_dbg_260_ee_mismatch  = 0;  // خواندنی: تعداد بایت‌های نخوانده در آخرین مقایسه (0 = همه درست)
volatile uint32_t mc_dbg_261_ee_last_addr = 0;  // خواندنی: آدرس آخرین کار
volatile uint32_t mc_dbg_262_ee_last_len  = 0;  // خواندنی: طول آخرین کار (بایت)
volatile uint32_t mc_dbg_263_ee_last_ms   = 0;  // خواندنی: مدت آخرین کار (میلی‌ثانیه)
volatile uint32_t mc_dbg_264_ee_rd_first  = 0;  // خواندنی: اولین بایت آخرین خواندن
volatile uint32_t mc_dbg_265_ee_rd_last   = 0;  // خواندنی: آخرین بایت آخرین خواندن
}

namespace mc {
namespace {

enum class Job : uint8_t { None = 0, Read = 1, Write = 2 };

struct Ctl {
    Job            job;
    EepromState    state;
    uint8_t        result;
    uint16_t       addr;        // آدرس شروع کار
    uint16_t       len;         // طول کل کار
    uint16_t       done;        // چند بایت انجام شده
    uint8_t*       rbuf;        // مقصد خواندن
    const uint8_t* wdata;       // منبع نوشتن
    bool           wait_ack;    // بعد از نوشتن، منتظر آماده شدن تراشه
    bool           present;     // تراشه جواب داده است
    bool           last_was_read;// آیا آخرین کار از نوع خواندن بود؟
    uint32_t       t_job_ms;    // زمان شروع کار
    uint32_t       t_write_ms;  // زمان آخرین قطعه نوشتن
    uint32_t       ok_cnt;
    uint32_t       err_cnt;
    uint32_t       refused_busy;
    uint32_t       last_ms;
    uint16_t       last_addr;
    uint16_t       last_len;
};

Ctl g = {Job::None, EepromState::Idle, kEepromResNone, 0U, 0U, 0U, nullptr, nullptr,
         false, false, false, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};

uint8_t g_tx[2U + MC_EEPROM_PAGE_BYTES];       // دو بایت آدرس + داده یک قطعه
uint8_t g_tbuf[MC_EEPROM_TEST_LEN];            // بافر ثابت فرمان‌های آزمایشی

// وضعیت مقایسه بعد از خواندن آزمایشی
bool     g_verify_pending = false;
uint16_t g_verify_len     = 0U;
uint8_t  g_verify_seed    = 0U;

// کار را با نتیجه می‌بندد.
void finish(bool ok, uint8_t res) {
    g.job      = Job::None;
    g.wait_ack = false;
    g.state    = ok ? EepromState::Done : EepromState::Error;
    g.result   = res;
    g.last_ms  = mc_millis() - g.t_job_ms;
    if (ok) { g.ok_cnt = g.ok_cnt + 1U; } else { g.err_cnt = g.err_cnt + 1U; }
}

// درخواست جدید را بررسی و ثبت می‌کند.
bool begin_job(Job job, uint16_t addr, uint16_t len, const void* ptr) {
    if (g.state == EepromState::Busy) {   // کار قبلی در جریان است
        g.refused_busy = g.refused_busy + 1U;
        return false;
    }
    g.t_job_ms      = mc_millis();
    g.last_addr     = addr;
    g.last_len      = len;
    g.last_was_read = (job == Job::Read);
    const uint32_t end = static_cast<uint32_t>(addr) + static_cast<uint32_t>(len);
    if ((ptr == nullptr) || (len == 0U) || (end > kEepromSizeBytes)) {
        finish(false, kEepromResRefused);
        return false;
    }
    if (!g.present) {
        finish(false, kEepromResNotFound);
        return false;
    }
    g.job      = job;
    g.addr     = addr;
    g.len      = len;
    g.done     = 0U;
    g.wait_ack = false;
    g.result   = kEepromResNone;
    g.state    = EepromState::Busy;
    return true;
}

// یک قطعه (حداکثر 32 بایت) از خواندن
void poll_read() {
    const uint16_t left = static_cast<uint16_t>(g.len - g.done);
    const uint16_t n    = (left > kEepromPageBytes) ? kEepromPageBytes : left;
    const uint16_t cur  = static_cast<uint16_t>(g.addr + g.done);
    g_tx[0] = static_cast<uint8_t>(cur >> 8);
    g_tx[1] = static_cast<uint8_t>(cur & 0xFFU);
    if (!mc_i2c_write(kEepromI2cAddr, g_tx, 2U, kEepromI2cTimeoutMs)) {   // تنظیم آدرس داخلی
        finish(false, kEepromResReadFail);
        return;
    }
    if (!mc_i2c_read(kEepromI2cAddr, &g.rbuf[g.done], n, kEepromI2cTimeoutMs)) {
        finish(false, kEepromResReadFail);
        return;
    }
    g.done = static_cast<uint16_t>(g.done + n);
    if (g.done >= g.len) { finish(true, kEepromResOk); }
}

// یک قطعه از نوشتن (بدون رد شدن از مرز صفحه) یا چک آماده شدن تراشه
void poll_write() {
    if (g.wait_ack) {
        if (mc_i2c_probe(kEepromI2cAddr, kEepromI2cTimeoutMs)) {   // تراشه نوشتن را تمام کرده
            g.wait_ack = false;
            if (g.done >= g.len) { finish(true, kEepromResOk); }
        } else if (mc_time_elapsed_ms(g.t_write_ms, kEepromWriteTimeoutMs)) {
            finish(false, kEepromResTimeout);
        }
        return;
    }
    const uint16_t cur  = static_cast<uint16_t>(g.addr + g.done);
    const uint16_t room = static_cast<uint16_t>(kEepromPageBytes - (cur % kEepromPageBytes));
    const uint16_t left = static_cast<uint16_t>(g.len - g.done);
    const uint16_t n    = (left > room) ? room : left;
    g_tx[0] = static_cast<uint8_t>(cur >> 8);
    g_tx[1] = static_cast<uint8_t>(cur & 0xFFU);
    for (uint16_t i = 0U; i < n; i++) { g_tx[2U + i] = g.wdata[g.done + i]; }
    if (!mc_i2c_write(kEepromI2cAddr, g_tx, static_cast<uint16_t>(2U + n), kEepromI2cTimeoutMs)) {
        finish(false, kEepromResWriteFail);
        return;
    }
    g.done       = static_cast<uint16_t>(g.done + n);
    g.wait_ack   = true;
    g.t_write_ms = mc_millis();
}

// ---------------- فرمان‌های آزمایشی ----------------
uint8_t pattern(uint16_t i, uint8_t seed) {
    return static_cast<uint8_t>(0x30U + i + seed);
}

void fill_pattern(uint16_t len, uint8_t seed) {
    for (uint16_t i = 0U; i < len; i++) { g_tbuf[i] = pattern(i, seed); }
}

// بعد از تمام شدن خواندن آزمایشی، با الگو مقایسه می‌کند.
void verify_check() {
    if (!g_verify_pending) { return; }
    if (g.state == EepromState::Busy) { return; }
    g_verify_pending = false;
    if (g.state != EepromState::Done) { return; }   // خواندن خودش خطا داشت
    uint32_t bad = 0U;
    for (uint16_t i = 0U; i < g_verify_len; i++) {
        if (g_tbuf[i] != pattern(i, g_verify_seed)) { bad = bad + 1U; }
    }
    mc_dbg_260_ee_mismatch = bad;
    if (bad != 0U) {
        g.state  = EepromState::Error;
        g.result = kEepromResVerify;
    }
}

void test_poll() {
    verify_check();
    const uint32_t cmd = mc_dbg_252_ee_cmd;
    if (cmd == 0U) { return; }
    mc_dbg_252_ee_cmd = 0U;   // هر فرمان فقط یک‌بار اجرا می‌شود
    const uint8_t  seed = static_cast<uint8_t>(mc_dbg_253_ee_seed & 0xFFU);
    const uint16_t mis_addr = static_cast<uint16_t>(MC_EEPROM_TEST_ADDR + MC_EEPROM_TEST_MIS_OFFSET);
    switch (cmd) {
        case 1U:   // ping: دوباره بررسی می‌کند تراشه جواب می‌دهد
            if (g.state != EepromState::Busy) {
                g.present = mc_i2c_probe(kEepromI2cAddr, kEepromI2cTimeoutMs);
            }
            break;
        case 2U:   // نوشتن مرتب: 64 بایت از ابتدای ناحیه تست
            fill_pattern(MC_EEPROM_TEST_LEN, seed);
            (void)mc_eeprom_write_start(MC_EEPROM_TEST_ADDR, g_tbuf, MC_EEPROM_TEST_LEN);
            break;
        case 3U:   // خواندن و مقایسه مرتب
            if (mc_eeprom_read_start(MC_EEPROM_TEST_ADDR, g_tbuf, MC_EEPROM_TEST_LEN)) {
                g_verify_pending = true; g_verify_len = MC_EEPROM_TEST_LEN; g_verify_seed = seed;
            }
            break;
        case 4U:   // نوشتن نامرتب: 40 بایت که از مرز صفحه رد می‌شود
            fill_pattern(MC_EEPROM_TEST_MIS_LEN, seed);
            (void)mc_eeprom_write_start(mis_addr, g_tbuf, MC_EEPROM_TEST_MIS_LEN);
            break;
        case 5U:   // خواندن و مقایسه نامرتب
            if (mc_eeprom_read_start(mis_addr, g_tbuf, MC_EEPROM_TEST_MIS_LEN)) {
                g_verify_pending = true; g_verify_len = MC_EEPROM_TEST_MIS_LEN; g_verify_seed = seed;
            }
            break;
        case 6U:   // نوشتن بیرون از حافظه: باید با نتیجه 3 رد شود و چیزی نوشته نشود
            fill_pattern(10U, seed);
            (void)mc_eeprom_write_start(static_cast<uint16_t>(kEepromSizeBytes - 6U), g_tbuf, 10U);
            break;
        case 7U:   // فقط خواندن 64 بایت ناحیه تست (بدون مقایسه)
            (void)mc_eeprom_read_start(MC_EEPROM_TEST_ADDR, g_tbuf, MC_EEPROM_TEST_LEN);
            break;
        default:
            break;
    }
}

void refresh_debug() {
    mc_dbg_254_ee_state     = static_cast<uint32_t>(g.state);
    mc_dbg_255_ee_result    = static_cast<uint32_t>(g.result);
    mc_dbg_256_ee_found     = g.present ? 1U : 0U;
    mc_dbg_257_ee_ok_cnt    = g.ok_cnt;
    mc_dbg_258_ee_err_cnt   = g.err_cnt;
    mc_dbg_259_ee_refused   = g.refused_busy;
    mc_dbg_261_ee_last_addr = g.last_addr;
    mc_dbg_262_ee_last_len  = g.last_len;
    mc_dbg_263_ee_last_ms   = g.last_ms;
    if (g.last_was_read && (g.state == EepromState::Done || g.state == EepromState::Error)) {
        if (g.last_len > 0U) {
            const uint16_t max_len = (g.last_len > MC_EEPROM_TEST_LEN) ? MC_EEPROM_TEST_LEN : g.last_len;
            mc_dbg_264_ee_rd_first = g_tbuf[0];
            mc_dbg_265_ee_rd_last  = g_tbuf[max_len - 1U];
        }
    }
}

}  // namespace

void mc_eeprom_init() {
    g.present = mc_i2c_probe(kEepromI2cAddr, kEepromI2cTimeoutMs);   // فقط probe، چیزی نوشته نمی‌شود
    refresh_debug();
}

void mc_eeprom_poll() {
    if (g.state == EepromState::Busy) {
        if (g.job == Job::Read)  { poll_read(); }
        if (g.job == Job::Write) { poll_write(); }
    }
    test_poll();
    refresh_debug();
}

bool mc_eeprom_present() { return g.present; }

bool mc_eeprom_read_start(uint16_t addr, uint8_t* buf, uint16_t len) {
    if (!begin_job(Job::Read, addr, len, buf)) { return false; }
    g.rbuf = buf;
    return true;
}

bool mc_eeprom_write_start(uint16_t addr, const uint8_t* data, uint16_t len) {
    if (!begin_job(Job::Write, addr, len, data)) { return false; }
    g.wdata = data;
    return true;
}

EepromState mc_eeprom_state()  { return g.state; }
uint8_t     mc_eeprom_result() { return g.result; }

}  // namespace mc
