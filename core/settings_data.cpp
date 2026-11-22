// جمع‌آوری، بررسی و اعمال تنظیمات فرمویر (بند ۷.۳) + متغیرهای دیباگ ۲۸۲ تا ۲۸۷.
#include "settings_data.h"
#include "motion_modes.h"
#include "motion_limits.h"
#include "planner.h"
#include "soft_limits.h"
#include "probe.h"
#include "extruder.h"
#include "heater_ex.h"
#include "../config/mc_config.h"
#include <cmath>
#include <cstring>

// ---------------- متغیرهای debug (Live Expressions در CubeIDE) ----------------
extern "C" {
volatile uint32_t mc_dbg_282_cfg_last_cmd   = 0;  // خواندنی: شماره آخرین دستور اجراشده (500 یا 501 یا 502 یا 503؛ 0 اگر هنوز هیچ‌کدام)
volatile uint32_t mc_dbg_283_cfg_result     = 0;  // خواندنی: نتیجه آخرین دستور (0 بیکار، 1 موفق، 2 خطای EEPROM یا verify، 3 هیچ اسلات سالم نیست، 4 نسخه یا اندازه نادرست، 5 مقدار نامعتبر، 6 زمان تمام شد)
volatile uint32_t mc_dbg_284_cfg_save_cnt   = 0;  // خواندنی: تعداد M500 موفق
volatile uint32_t mc_dbg_285_cfg_load_cnt   = 0;  // خواندنی: تعداد M501 موفق (که اعمال شد)
volatile uint32_t mc_dbg_286_cfg_len        = 0;  // خواندنی: طول payload آخرین M500 یا M501 (بایت)
volatile uint32_t mc_dbg_287_cfg_store_res = 0;  // خواندنی: آخرین مقدار mc_settings_result() دیده‌شده
}

namespace mc {
namespace {

constexpr uint16_t kPayloadVersion = MC_CFG_PAYLOAD_VERSION;

// چیدمان بایتی payload (طول کل: 86 بایت):
// Off 0: version (uint16_t, 2 بایت)
// Off 2: total len (uint16_t, 2 بایت)
// Off 4: spmm X, Y, Z, E (4 * float = 16 بایت)
// Off 20: max feed X, Y, Z, E (4 * float = 16 بایت)
// Off 36: accel print, retract, travel (3 * float = 12 بایت)
// Off 48: min feed (float, 4 بایت)
// Off 52: junction deviation um (uint32_t, 4 بایت)
// Off 56: softlimit enabled (uint8_t, 1 بایت)
// Off 57: probe offset X, Y, Z (3 * float = 12 بایت)
// Off 69: extruder min temp x10 (int32_t, 4 بایت)
// Off 73: extruder cold ok (uint8_t, 1 بایت)
// Off 74: PID Kp, Ki, Kd x100 (3 * uint32_t = 12 بایت)
constexpr uint16_t kPayloadLen = 86U;

static_assert(kPayloadLen <= kSettingsPayloadMax, "طول payload از حداکثر ظرفیت اسلات بیشتر است");

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

void put_float(uint8_t* p, float v) {
    static_assert(sizeof(float) == 4, "float باید 4 بایت باشد");
    uint32_t u = 0U;
    std::memcpy(&u, &v, sizeof(u));
    put32(p, u);
}

float get_float(const uint8_t* p) {
    static_assert(sizeof(float) == 4, "float باید 4 بایت باشد");
    uint32_t u = get32(p);
    float v = 0.0f;
    std::memcpy(&v, &u, sizeof(v));
    return v;
}

}  // namespace

void mc_cfg_note(uint32_t cmd, uint8_t result, uint16_t len, uint8_t store_res) {
    mc_dbg_282_cfg_last_cmd   = cmd;
    mc_dbg_283_cfg_result     = static_cast<uint32_t>(result);
    mc_dbg_286_cfg_len        = static_cast<uint32_t>(len);
    mc_dbg_287_cfg_store_res = static_cast<uint32_t>(store_res);
    if (result == 1U) {
        if (cmd == 500U) { mc_dbg_284_cfg_save_cnt = mc_dbg_284_cfg_save_cnt + 1U; }
        if (cmd == 501U) { mc_dbg_285_cfg_load_cnt = mc_dbg_285_cfg_load_cnt + 1U; }
    }
}

uint16_t mc_cfg_collect(uint8_t* out, uint16_t max_len) {
    if ((out == nullptr) || (max_len < kPayloadLen)) { return 0U; }

    put16(&out[0], kPayloadVersion);
    put16(&out[2], kPayloadLen);

    // ۱. گام بر میلی‌متر (M92)
    for (uint8_t i = 0U; i < 4U; i++) { put_float(&out[4U + (i * 4U)], mc_mode_spmm_get(i)); }

    // ۲. حداکثر سرعت (M203)
    for (uint8_t i = 0U; i < 4U; i++) { put_float(&out[20U + (i * 4U)], mc_mlim_feed_get(i)); }

    // ۳. شتاب (M204)
    for (uint8_t i = 0U; i < 3U; i++) { put_float(&out[36U + (i * 4U)], mc_mlim_acc_get(i)); }

    // ۴. حداقل سرعت (M205 S)
    put_float(&out[48U], mc_mlim_minfeed_get());

    // ۵. انحراف اتصال (M205 J)
    put32(&out[52U], mc_plan_jd_get());

    // ۶. حدود نرم‌افزاری (M211 S)
    out[56U] = mc_softlimit_enabled_get() ? 1U : 0U;

    // ۷. آفست پراب (M851)
    for (uint8_t i = 0U; i < 3U; i++) { put_float(&out[57U + (i * 4U)], mc_probe_offset_get(i)); }

    // ۸. دمای حداقل اکستروژن و اکستروژن سرد (M302)
    put32(&out[69U], static_cast<uint32_t>(mc_extruder_min_temp_x10_get()));
    out[73U] = mc_extruder_cold_ok_get() ? 1U : 0U;

    // ۹. ضرایب PID هات‌اند
    uint32_t kp = 0U, ki = 0U, kd = 0U;
    mc_heater_ex_get_gains_x100(kp, ki, kd);
    put32(&out[74U], kp);
    put32(&out[78U], ki);
    put32(&out[82U], kd);

    return kPayloadLen;
}

uint8_t mc_cfg_check(const uint8_t* in, uint16_t len) {
    if ((in == nullptr) || (len != kPayloadLen)) { return 4U; }
    if (get16(&in[0]) != kPayloadVersion) { return 4U; }
    if (get16(&in[2]) != kPayloadLen) { return 4U; }

    // ۱. بررسی M92
    for (uint8_t i = 0U; i < 4U; i++) {
        const float v = get_float(&in[4U + (i * 4U)]);
        if (std::isnan(v) || (v < kModeMinStepsPerMm) || (v > kModeMaxStepsPerMm)) { return 5U; }
    }

    // ۲. بررسی M203
    for (uint8_t i = 0U; i < 4U; i++) {
        const float v = get_float(&in[20U + (i * 4U)]);
        if (std::isnan(v) || (v < kMlimMinFeedMmS) || (v > kMlimMaxFeedMmS)) { return 5U; }
    }

    // ۳. بررسی M204
    for (uint8_t i = 0U; i < 3U; i++) {
        const float v = get_float(&in[36U + (i * 4U)]);
        if (std::isnan(v) || (v < kMlimMinAccMmS2) || (v > kMlimMaxAccMmS2)) { return 5U; }
    }

    // ۴. بررسی M205 S
    const float minfeed = get_float(&in[48U]);
    if (std::isnan(minfeed) || (minfeed < kMlimMinMinFeedMmS) || (minfeed > kMlimMaxMinFeedMmS)) { return 5U; }

    // ۵. بررسی M205 J
    const uint32_t jd_um = get32(&in[52U]);
    if ((jd_um < kPlanMinJunctionUm) || (jd_um > kPlanMaxJunctionUm)) { return 5U; }

    // ۶. بررسی M211 S
    if (in[56U] > 1U) { return 5U; }

    // ۷. بررسی M851
    for (uint8_t i = 0U; i < 3U; i++) {
        const float off = get_float(&in[57U + (i * 4U)]);
        if (!mc_probe_offset_valid(i, off)) { return 5U; }
    }

    // ۸. بررسی M302
    const int32_t min_t_x10 = static_cast<int32_t>(get32(&in[69U]));
    if ((min_t_x10 < 0) || (min_t_x10 > (kExMaxMinTempDeg * 10))) { return 5U; }
    if (in[73U] > 1U) { return 5U; }

    // ۹. بررسی PID
    const uint32_t kp_x100 = get32(&in[74U]);
    const uint32_t ki_x100 = get32(&in[78U]);
    const uint32_t kd_x100 = get32(&in[82U]);
    const float kp = static_cast<float>(kp_x100) / 100.0f;
    const float ki = static_cast<float>(ki_x100) / 100.0f;
    const float kd = static_cast<float>(kd_x100) / 100.0f;
    if (std::isnan(kp) || std::isnan(ki) || std::isnan(kd)) { return 5U; }
    if ((kp < 0.1f) || (kp > 1000.0f) || (ki <= 0.0f) || (ki >= 1000000.0f) || (kd <= 0.0f) || (kd >= 1000000.0f)) {
        return 5U;
    }

    return 0U;
}

bool mc_cfg_apply(const uint8_t* in, uint16_t len) {
    if (mc_cfg_check(in, len) != 0U) { return false; }

    // ۱. M92
    for (uint8_t i = 0U; i < 4U; i++) { (void)mc_mode_spmm_set(i, get_float(&in[4U + (i * 4U)])); }

    // ۲. M203
    for (uint8_t i = 0U; i < 4U; i++) { (void)mc_mlim_feed_set(i, get_float(&in[20U + (i * 4U)])); }

    // ۳. M204
    for (uint8_t i = 0U; i < 3U; i++) { (void)mc_mlim_acc_set(i, get_float(&in[36U + (i * 4U)])); }

    // ۴. M205 S
    (void)mc_mlim_minfeed_set(get_float(&in[48U]));

    // ۵. M205 J
    (void)mc_plan_jd_set(get32(&in[52U]));

    // ۶. M211 S
    mc_softlimit_enabled_set(in[56U] != 0U);

    // ۷. M851
    for (uint8_t i = 0U; i < 3U; i++) { (void)mc_probe_offset_set(i, get_float(&in[57U + (i * 4U)])); }

    // ۸. M302
    (void)mc_extruder_min_temp_x10_set(static_cast<int32_t>(get32(&in[69U])));
    mc_extruder_cold_ok_set(in[73U] != 0U);

    // ۹. PID
    mc_heater_ex_set_gains_x100(get32(&in[74U]), get32(&in[78U]), get32(&in[82U]));

    return true;
}

void mc_cfg_apply_defaults() {
    // ۱. M92
    for (uint8_t i = 0U; i < 4U; i++) { (void)mc_mode_spmm_set(i, kModeDefaultStepsPerMm[i]); }

    // ۲. M203
    (void)mc_mlim_feed_set(0U, kMlimDefaultFeedXMmS);
    (void)mc_mlim_feed_set(1U, kMlimDefaultFeedYMmS);
    (void)mc_mlim_feed_set(2U, kMlimDefaultFeedZMmS);
    (void)mc_mlim_feed_set(3U, kMlimDefaultFeedEMmS);

    // ۳. M204
    (void)mc_mlim_acc_set(0U, kMlimDefaultAccPrint);
    (void)mc_mlim_acc_set(1U, kMlimDefaultAccRetract);
    (void)mc_mlim_acc_set(2U, kMlimDefaultAccTravel);

    // ۴. M205 S
    (void)mc_mlim_minfeed_set(kMlimDefaultMinFeedMmS);

    // ۵. M205 J
    (void)mc_plan_jd_set(kPlanDefaultJunctionUm);

    // ۶. M211 S
    mc_softlimit_enabled_set(true);

    // ۷. M851
    (void)mc_probe_offset_set(0U, MC_PROBE_OFFSET_X_MM);
    (void)mc_probe_offset_set(1U, MC_PROBE_OFFSET_Y_MM);
    (void)mc_probe_offset_set(2U, MC_PROBE_OFFSET_Z_MM);

    // ۸. M302
    (void)mc_extruder_min_temp_x10_set(kExDefaultMinTempX10);
    mc_extruder_cold_ok_set(false);

    // ۹. PID
    mc_heater_ex_set_gains_x100(
        static_cast<uint32_t>((MC_HOTEND_PID_KP * 100.0f) + 0.5f),
        static_cast<uint32_t>((MC_HOTEND_PID_KI * 100.0f) + 0.5f),
        static_cast<uint32_t>((MC_HOTEND_PID_KD * 100.0f) + 0.5f)
    );
}

}  // namespace mc
