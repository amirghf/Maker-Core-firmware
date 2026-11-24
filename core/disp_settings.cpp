// ذخیره و بارگذاری تنظیمات نمایشگر روی EEPROM (بند ۸.۲۱).
// این رکورد مربوط به رابط کاربری است؛ M500 و M502 به آن دست نمی‌زنند.
// هیچ هدر ST اینجا include نمی‌شود.
#include "disp_settings.h"
#include "eeprom_at24.h"
#include "crc32.h"
#include "settings_store.h"
#include "settings_boot.h"
#include "touch_cal_store.h"
#include "hal_time.h"
#include <string.h>

namespace mc {
namespace {

enum class Job : uint8_t { None = 0, Load = 1, Save = 2, Clear = 3 };
enum class SubStep : uint8_t { WaitStart = 0, WaitEepromWrite = 1, WaitEepromRead = 2, WaitVerifyStart = 3, WaitVerifyRead = 4 };

constexpr uint16_t kRecLen = 20U;   // طول رکورد (بایت)
constexpr uint16_t kCrcLen = 16U;   // CRC روی این تعداد بایت اول محاسبه می‌شود

Job      g_job     = Job::None;
uint8_t  g_state   = 0U;   // 0 بیکار، 1 در جریان، 2 موفق، 3 خطا
uint8_t  g_result  = 0U;
SubStep  g_substep = SubStep::WaitStart;
uint32_t g_t_start = 0U;

uint8_t  g_rec_buf[kRecLen];
uint8_t  g_wbuf[kRecLen];

uint32_t g_screen_off_s = MC_UI_SCREEN_SLEEP_MS / 1000U;   // مدت بیکاری تا خاموشی (ثانیه)، 0 = هیچ‌وقت
uint32_t g_menu_exit_s  = MC_UI_MENU_EXIT_MS / 1000U;      // مدت بیکاری تا خروج از منو (ثانیه)، 0 = هیچ‌وقت

void pack_uint32(uint8_t* dst, uint32_t val) {
    dst[0] = static_cast<uint8_t>(val & 0xFFU);
    dst[1] = static_cast<uint8_t>((val >> 8) & 0xFFU);
    dst[2] = static_cast<uint8_t>((val >> 16) & 0xFFU);
    dst[3] = static_cast<uint8_t>((val >> 24) & 0xFFU);
}

uint32_t unpack_uint32(const uint8_t* src) {
    return static_cast<uint32_t>(src[0]) |
          (static_cast<uint32_t>(src[1]) << 8) |
          (static_cast<uint32_t>(src[2]) << 16) |
          (static_cast<uint32_t>(src[3]) << 24);
}

// پایان کار با وضعیت و نتیجه
void finish(uint8_t state, uint8_t result) {
    g_state  = state;
    g_result = result;
    g_job    = Job::None;
}

// آیا الان اجازه شروع کار تازه نیست؟
bool start_blocked() {
    return mc_cfg_boot_busy() || (mc_settings_state() == SettingsState::Busy) ||
           (g_state == 1U) || (mc_tcal_store_state() == 1U);
}

void begin(Job job) {
    g_job     = job;
    g_state   = 1U;
    g_result  = 0U;
    g_substep = SubStep::WaitStart;
    g_t_start = mc_millis();
}

}  // namespace

void mc_dispset_init() {
    g_job     = Job::None;
    g_state   = 0U;
    g_result  = 0U;
    g_substep = SubStep::WaitStart;
    g_screen_off_s = MC_UI_SCREEN_SLEEP_MS / 1000U;
    g_menu_exit_s  = MC_UI_MENU_EXIT_MS / 1000U;
}

void mc_dispset_poll() {
    if (g_state != 1U) { return; }   // فقط وقتی کار در جریان است

    if (mc_time_elapsed_ms(g_t_start, MC_DISPSET_TIMEOUT_MS)) {
        finish(3U, 2U);
        return;
    }

    const EepromState ee_st = mc_eeprom_state();

    if (g_job == Job::Load) {
        if (g_substep == SubStep::WaitStart) {
            if ((mc_settings_state() == SettingsState::Busy) || (ee_st == EepromState::Busy)) { return; }
            if (mc_eeprom_read_start(kDispSetAddr, g_rec_buf, kRecLen)) {
                g_substep = SubStep::WaitEepromRead;
            }
            return;
        }
        if (g_substep == SubStep::WaitEepromRead) {
            if (ee_st == EepromState::Busy) { return; }
            if (ee_st == EepromState::Error) { finish(3U, 2U); return; }
            if (ee_st == EepromState::Done) {
                const uint32_t magic    = unpack_uint32(&g_rec_buf[0]);
                const uint8_t  ver      = g_rec_buf[4];
                const uint32_t crc_rec  = unpack_uint32(&g_rec_buf[16]);
                const uint32_t crc_calc = mc_crc32(g_rec_buf, kCrcLen);
                if ((magic != MC_DISPSET_MAGIC) || (ver != MC_DISPSET_FORMAT_VERSION) || (crc_rec != crc_calc)) {
                    finish(3U, 3U);   // رکورد معتبر نیست
                    return;
                }
                const uint32_t scr_s  = unpack_uint32(&g_rec_buf[8]);
                const uint32_t menu_s = unpack_uint32(&g_rec_buf[12]);
                if ((scr_s > MC_UI_DISP_MAX_S) || (menu_s > MC_UI_DISP_MAX_S)) {
                    finish(3U, 3U);   // مقدار نامعقول
                    return;
                }
                g_screen_off_s = scr_s;
                g_menu_exit_s  = menu_s;
                finish(2U, 1U);
                return;
            }
        }
        return;
    }

    if ((g_job == Job::Save) || (g_job == Job::Clear)) {
        if (g_substep == SubStep::WaitStart) {
            if ((mc_settings_state() == SettingsState::Busy) || (ee_st == EepromState::Busy)) { return; }
            if (mc_eeprom_write_start(kDispSetAddr, g_wbuf, kRecLen)) {
                g_substep = SubStep::WaitEepromWrite;
            }
            return;
        }
        if (g_substep == SubStep::WaitEepromWrite) {
            if (ee_st == EepromState::Busy) { return; }
            if (ee_st == EepromState::Error) { finish(3U, 2U); return; }
            if (ee_st == EepromState::Done) {
                if (g_job == Job::Clear) { finish(2U, 1U); return; }
                g_substep = SubStep::WaitVerifyStart;
            }
            return;
        }
        if (g_substep == SubStep::WaitVerifyStart) {
            if (ee_st == EepromState::Busy) { return; }
            if (mc_eeprom_read_start(kDispSetAddr, g_rec_buf, kRecLen)) {
                g_substep = SubStep::WaitVerifyRead;
            }
            return;
        }
        if (g_substep == SubStep::WaitVerifyRead) {
            if (ee_st == EepromState::Busy) { return; }
            if (ee_st == EepromState::Error) { finish(3U, 2U); return; }
            if (ee_st == EepromState::Done) {
                if (memcmp(g_rec_buf, g_wbuf, kRecLen) != 0) { finish(3U, 4U); return; }
                finish(2U, 1U);
            }
            return;
        }
    }
}

uint32_t mc_dispset_screen_off_s() { return g_screen_off_s; }
uint32_t mc_dispset_menu_exit_s()  { return g_menu_exit_s; }

bool mc_dispset_set_values(uint32_t screen_off_s, uint32_t menu_exit_s) {
    if ((screen_off_s > MC_UI_DISP_MAX_S) || (menu_exit_s > MC_UI_DISP_MAX_S)) { return false; }
    g_screen_off_s = screen_off_s;
    g_menu_exit_s  = menu_exit_s;
    return true;
}

bool mc_dispset_load_start() {
    if (start_blocked()) { return false; }
    begin(Job::Load);
    return true;
}

bool mc_dispset_save_start() {
    if (start_blocked()) { return false; }
    memset(g_wbuf, 0, sizeof(g_wbuf));
    pack_uint32(&g_wbuf[0], MC_DISPSET_MAGIC);
    g_wbuf[4] = static_cast<uint8_t>(MC_DISPSET_FORMAT_VERSION);
    // 3 بایت رزرو (بایت 5 تا 7) صفر می‌ماند
    pack_uint32(&g_wbuf[8], g_screen_off_s);
    pack_uint32(&g_wbuf[12], g_menu_exit_s);
    pack_uint32(&g_wbuf[16], mc_crc32(g_wbuf, kCrcLen));
    begin(Job::Save);
    return true;
}

bool mc_dispset_clear_start() {
    if (start_blocked()) { return false; }
    memset(g_wbuf, 0, sizeof(g_wbuf));   // magic صفر = رکورد نامعتبر
    begin(Job::Clear);
    return true;
}

uint8_t mc_dispset_state()  { return g_state; }
uint8_t mc_dispset_result() { return g_result; }

}  // namespace mc
