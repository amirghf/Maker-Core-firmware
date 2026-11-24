// ذخیره و بارگذاری ضرایب کالیبراسیون لمس روی EEPROM (بند ۸.۱۵).
// کالیبراسیون لمس مربوط به سخت‌افزار برد و نمایشگر است، نه تنظیمات چاپ؛
// بنابراین M500 و M502 و بازگشت به کارخانه این رکورد را نمی‌خوانند، نمی‌نویسند و پاک نمی‌کنند.
// هیچ هدر ST اینجا include نمی‌شود.
#include "touch_cal_store.h"
#include "eeprom_at24.h"
#include "crc32.h"
#include "touch.h"
#include "settings_store.h"
#include "settings_boot.h"
#include "hal_time.h"
#include <string.h>

namespace mc {
namespace {

enum class Job : uint8_t { None = 0, Load = 1, Save = 2, Clear = 3 };
enum class SubStep : uint8_t { WaitStart = 0, WaitEepromWrite = 1, WaitEepromRead = 2, WaitVerifyStart = 3, WaitVerifyRead = 4 };

Job      g_job       = Job::None;
uint8_t  g_state     = 0U;   // 0 بیکار، 1 در جریان، 2 موفق، 3 خطا
uint8_t  g_result    = 0U;   // 0 هیچ، 1 موفق، 2 خطا/عدم پاسخ، 3 رکورد نامعتبر، 4 verify ناموفق، 5 درخواست نامعتبر
SubStep  g_substep   = SubStep::WaitStart;
uint32_t g_t_start   = 0U;

uint8_t  g_rec_buf[36];
uint8_t  g_wbuf[36];

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

}  // namespace

void mc_tcal_store_init() {
    g_job     = Job::None;
    g_state   = 0U;
    g_result  = 0U;
    g_substep = SubStep::WaitStart;
}

void mc_tcal_store_poll() {
    if (g_state != 1U) { return; }   // فقط وقتی کار در جریان است

    if (mc_time_elapsed_ms(g_t_start, MC_TOUCH_CAL_STORE_TIMEOUT_MS)) {
        g_result = 2U;   // EEPROM timeout / error
        g_state  = 3U;   // Error
        g_job    = Job::None;
        return;
    }

    const EepromState ee_st = mc_eeprom_state();

    if (g_job == Job::Load) {
        if (g_substep == SubStep::WaitStart) {
            if ((mc_settings_state() == SettingsState::Busy) || (ee_st == EepromState::Busy)) {
                return;   // در دور بعد دوباره تلاش کن
            }
            if (mc_eeprom_read_start(kTouchCalAddr, g_rec_buf, sizeof(g_rec_buf))) {
                g_substep = SubStep::WaitEepromRead;
            }
            return;
        }

        if (g_substep == SubStep::WaitEepromRead) {
            if (ee_st == EepromState::Busy) { return; }
            if (ee_st == EepromState::Error) {
                g_result = 2U;
                g_state  = 3U;
                g_job    = Job::None;
                return;
            }
            if (ee_st == EepromState::Done) {
                const uint32_t magic = unpack_uint32(&g_rec_buf[0]);
                const uint8_t  ver   = g_rec_buf[4];
                const uint32_t crc_rec = unpack_uint32(&g_rec_buf[32]);
                const uint32_t crc_calc = mc_crc32(g_rec_buf, 28U);

                if ((magic != MC_TOUCH_CAL_MAGIC) || (ver != MC_TOUCH_CAL_FORMAT_VERSION) || (crc_rec != crc_calc)) {
                    g_result = 3U;   // رکورد معتبر نیست
                    g_state  = 3U;
                    g_job    = Job::None;
                    return;
                }

                float coef[6];
                memcpy(coef, &g_rec_buf[8], sizeof(coef));
                if (!mc_touch_set_coef(coef)) {
                    g_result = 3U;   // ضرایب نامعقول
                    g_state  = 3U;
                    g_job    = Job::None;
                    return;
                }

                g_result = 1U;   // موفق
                g_state  = 2U;   // Done
                g_job    = Job::None;
                return;
            }
        }
    }

    if (g_job == Job::Save) {
        if (g_substep == SubStep::WaitStart) {
            if ((mc_settings_state() == SettingsState::Busy) || (ee_st == EepromState::Busy)) {
                return;
            }
            if (mc_eeprom_write_start(kTouchCalAddr, g_wbuf, sizeof(g_wbuf))) {
                g_substep = SubStep::WaitEepromWrite;
            }
            return;
        }

        if (g_substep == SubStep::WaitEepromWrite) {
            if (ee_st == EepromState::Busy) { return; }
            if (ee_st == EepromState::Error) {
                g_result = 2U;
                g_state  = 3U;
                g_job    = Job::None;
                return;
            }
            if (ee_st == EepromState::Done) {
                g_substep = SubStep::WaitVerifyStart;
                return;
            }
        }

        if (g_substep == SubStep::WaitVerifyStart) {
            if (ee_st == EepromState::Busy) { return; }
            if (mc_eeprom_read_start(kTouchCalAddr, g_rec_buf, sizeof(g_rec_buf))) {
                g_substep = SubStep::WaitVerifyRead;
            }
            return;
        }

        if (g_substep == SubStep::WaitVerifyRead) {
            if (ee_st == EepromState::Busy) { return; }
            if (ee_st == EepromState::Error) {
                g_result = 2U;
                g_state  = 3U;
                g_job    = Job::None;
                return;
            }
            if (ee_st == EepromState::Done) {
                if (memcmp(g_rec_buf, g_wbuf, sizeof(g_wbuf)) != 0) {
                    g_result = 4U;   // verify ناموفق
                    g_state  = 3U;
                    g_job    = Job::None;
                    return;
                }
                g_result = 1U;   // موفق
                g_state  = 2U;   // Done
                g_job    = Job::None;
                return;
            }
        }
    }

    if (g_job == Job::Clear) {
        if (g_substep == SubStep::WaitStart) {
            if ((mc_settings_state() == SettingsState::Busy) || (ee_st == EepromState::Busy)) {
                return;
            }
            if (mc_eeprom_write_start(kTouchCalAddr, g_wbuf, sizeof(g_wbuf))) {
                g_substep = SubStep::WaitEepromWrite;
            }
            return;
        }

        if (g_substep == SubStep::WaitEepromWrite) {
            if (ee_st == EepromState::Busy) { return; }
            if (ee_st == EepromState::Error) {
                g_result = 2U;
                g_state  = 3U;
                g_job    = Job::None;
                return;
            }
            if (ee_st == EepromState::Done) {
                g_result = 1U;   // موفق
                g_state  = 2U;   // Done
                g_job    = Job::None;
                return;
            }
        }
    }
}

bool mc_tcal_store_load_start() {
    if (mc_cfg_boot_busy() || (mc_settings_state() == SettingsState::Busy) || (g_state == 1U)) {
        return false;
    }
    g_job     = Job::Load;
    g_state   = 1U;
    g_result  = 0U;
    g_substep = SubStep::WaitStart;
    g_t_start = mc_millis();
    return true;
}

bool mc_tcal_store_save_start() {
    if (mc_cfg_boot_busy() || (mc_settings_state() == SettingsState::Busy) || (g_state == 1U)) {
        return false;
    }
    float coef[6];
    if (!mc_touch_get_coef(coef)) {
        g_result = 5U;   // درخواست نامعتبر (کالیبره نیست)
        return false;
    }

    memset(g_wbuf, 0, sizeof(g_wbuf));
    pack_uint32(&g_wbuf[0], MC_TOUCH_CAL_MAGIC);
    g_wbuf[4] = static_cast<uint8_t>(MC_TOUCH_CAL_FORMAT_VERSION);
    // 3 بایت رزرو (بایت 5 تا 7) صفر می‌ماند
    memcpy(&g_wbuf[8], coef, sizeof(coef));
    const uint32_t crc = mc_crc32(g_wbuf, 28U);
    pack_uint32(&g_wbuf[32], crc);

    g_job     = Job::Save;
    g_state   = 1U;
    g_result  = 0U;
    g_substep = SubStep::WaitStart;
    g_t_start = mc_millis();
    return true;
}

bool mc_tcal_store_clear_start() {
    if (mc_cfg_boot_busy() || (mc_settings_state() == SettingsState::Busy) || (g_state == 1U)) {
        return false;
    }
    memset(g_wbuf, 0, sizeof(g_wbuf));   // magic صفر می‌نویسد تا رکورد نامعتبر شود

    g_job     = Job::Clear;
    g_state   = 1U;
    g_result  = 0U;
    g_substep = SubStep::WaitStart;
    g_t_start = mc_millis();
    return true;
}

uint8_t mc_tcal_store_state()  { return g_state; }
uint8_t mc_tcal_store_result() { return g_result; }

}  // namespace mc
