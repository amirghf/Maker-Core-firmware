// خواندن خودکار تنظیمات هنگام بوت از EEPROM (بند ۷.۴) + متغیرهای دیباگ ۲۸۸ تا ۲۹۲.
#include "settings_boot.h"
#include "settings_store.h"
#include "settings_data.h"
#include "motion_queue.h"
#include "log.h"
#include "hal_time.h"

// ---------------- متغیرهای debug (Live Expressions در CubeIDE) ----------------
extern "C" {
volatile uint32_t mc_dbg_288_boot_state     = 0;  // خواندنی: وضعیت ماژول بوت (0 شروع‌نشده، 1 در جریان، 2 تمام)
volatile uint32_t mc_dbg_289_boot_outcome   = 0;  // خواندنی: نتیجه بوت (0 هنوز تمام نشده، 1 تنظیمات ذخیره‌شده اعمال شد، 2 هیچ اسلات سالم نیست و پیش‌فرض ماند، 3 داده نامعتبر و پیش‌فرض ماند، 4 خطای EEPROM یا زمان و پیش‌فرض ماند)
volatile uint32_t mc_dbg_290_boot_store_res = 0;  // خواندنی: آخرین مقدار mc_settings_result() دیده‌شده در بوت
volatile uint32_t mc_dbg_291_boot_check     = 0;  // خواندنی: نتیجه mc_cfg_check در بوت (0 معتبر، 4 اندازه یا نسخه نادرست، 5 مقدار نامعتبر؛ 0 اگر چک انجام نشد)
volatile uint32_t mc_dbg_292_boot_len       = 0;  // خواندنی: طول داده خوانده‌شده در بوت (بایت؛ 0 اگر چیزی خوانده نشد)
}

namespace mc {
namespace {

enum class BootState : uint8_t { Idle = 0, Loading = 1, Done = 2 };

BootState g_state   = BootState::Idle;
uint32_t  g_t_start = 0U;
uint8_t   g_buf[kSettingsPayloadMax];

void update_debug(BootState st, uint32_t outcome, uint32_t store_res, uint32_t chk_res, uint32_t len) {
    g_state = st;
    mc_dbg_288_boot_state     = static_cast<uint32_t>(st);
    mc_dbg_289_boot_outcome   = outcome;
    mc_dbg_290_boot_store_res = store_res;
    mc_dbg_291_boot_check     = chk_res;
    mc_dbg_292_boot_len       = len;
}

}  // namespace

void mc_cfg_boot_init() {
    update_debug(BootState::Idle, 0U, 0U, 0U, 0U);
}

void mc_cfg_boot_poll() {
    if (g_state == BootState::Done) {
        return;
    }

    if (g_state == BootState::Idle) {
        g_t_start = mc_millis();
        update_debug(BootState::Loading, 0U, 0U, 0U, 0U);
        (void)mc_settings_load_start(g_buf, static_cast<uint16_t>(sizeof(g_buf)));
        return;
    }

    // g_state == BootState::Loading
    if (mc_time_elapsed_ms(g_t_start, MC_CFG_BOOT_TIMEOUT_MS)) {
        mc_cfg_apply_defaults();
        mc_log(LogLevel::Info, "Settings: EEPROM error, defaults used");
        update_debug(BootState::Done, 4U, mc_settings_result(), 0U, 0U);
        return;
    }

    const SettingsState st = mc_settings_state();
    if (st == SettingsState::Idle) {
        // اگر هنوز شروع نشده یا درخواست قبلی رد شده، دوباره درخواست بده
        (void)mc_settings_load_start(g_buf, static_cast<uint16_t>(sizeof(g_buf)));
        return;
    }

    if (st == SettingsState::Busy) {
        return;
    }

    if (st == SettingsState::Done) {
        const uint16_t loaded_len = mc_settings_loaded_len();
        const uint8_t chk = mc_cfg_check(g_buf, loaded_len);
        if (chk == 0U) {
            if (mc_mq_busy()) {
                return;   // صبر کن تا صف حرکت خالی شود
            }
            (void)mc_cfg_apply(g_buf, loaded_len);
            mc_log(LogLevel::Info, "Settings loaded from EEPROM");
            update_debug(BootState::Done, 1U, mc_settings_result(), 0U, loaded_len);
            return;
        }

        mc_cfg_apply_defaults();
        mc_log(LogLevel::Info, "Settings: saved data invalid, defaults used");
        update_debug(BootState::Done, 3U, mc_settings_result(), static_cast<uint32_t>(chk), loaded_len);
        return;
    }

    // st == SettingsState::Error
    const uint8_t store_res = mc_settings_result();
    mc_cfg_apply_defaults();
    if (store_res == kSetResNoValid) {
        mc_log(LogLevel::Info, "Settings: no saved data, defaults used");
        update_debug(BootState::Done, 2U, store_res, 0U, 0U);
    } else {
        mc_log(LogLevel::Info, "Settings: EEPROM error, defaults used");
        update_debug(BootState::Done, 4U, store_res, 0U, 0U);
    }
}

bool mc_cfg_boot_busy() {
    return (g_state != BootState::Done);
}

}  // namespace mc
