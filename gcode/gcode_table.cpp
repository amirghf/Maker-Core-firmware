#include "gcode_handlers.h"

namespace mc {
namespace {

// جدول دستورها. برای افزودن دستور جدید: یک فایل در gcode بساز و یک خط به این جدول اضافه کن.
const GcodeEntry kTable[] = {
    // حرف، شماره، زیرکد (-۱ = بدون زیرکد)، تابع
    { 'G', 0U,   -1, mc_gcode_g0 },
    { 'G', 1U,   -1, mc_gcode_g1 },
    { 'G', 4U,   -1, mc_gcode_g4 },
    { 'G', 28U,  -1, mc_gcode_g28 },
    { 'G', 30U,  -1, mc_gcode_g30 },
    { 'G', 90U,  -1, mc_gcode_g90 },
    { 'G', 91U,  -1, mc_gcode_g91 },
    { 'G', 92U,  -1, mc_gcode_g92 },
    { 'M', 17U,  -1, mc_gcode_m17 },
    { 'M', 18U,  -1, mc_gcode_m18 },
    { 'M', 82U,  -1, mc_gcode_m82 },
    { 'M', 83U,  -1, mc_gcode_m83 },
    { 'M', 84U,  -1, mc_gcode_m84 },
    { 'M', 92U,  -1, mc_gcode_m92 },
    { 'M', 203U, -1, mc_gcode_m203 },
    { 'M', 204U, -1, mc_gcode_m204 },
    { 'M', 205U, -1, mc_gcode_m205 },
    { 'M', 211U, -1, mc_gcode_m211 },
    { 'M', 280U, -1, mc_gcode_m280 },
    { 'M', 401U, -1, mc_gcode_m401 },
    { 'M', 402U, -1, mc_gcode_m402 },
    { 'M', 851U, -1, mc_gcode_m851 },
    { 'M', 302U, -1, mc_gcode_m302 },
    { 'M', 104U, -1, mc_gcode_m104 },
    { 'M', 105U, -1, mc_gcode_m105 },
    { 'M', 106U, -1, mc_gcode_m106 },
    { 'M', 107U, -1, mc_gcode_m107 },
    { 'M', 108U, -1, mc_gcode_m108 },
    { 'M', 109U, -1, mc_gcode_m109 },
    { 'M', 110U, -1, mc_gcode_m110 },
    { 'M', 114U, -1, mc_gcode_m114 },
    { 'M', 115U, -1, mc_gcode_m115 },
    { 'M', 119U, -1, mc_gcode_m119 },
    { 'M', 140U, -1, mc_gcode_m140 },
    { 'M', 155U, -1, mc_gcode_m155 },
    { 'M', 190U, -1, mc_gcode_m190 },
    { 'M', 301U, -1, mc_gcode_m301 },
    { 'M', 303U, -1, mc_gcode_m303 },
    { 'M', 400U, -1, mc_gcode_m400 },
    { 'M', 500U, -1, mc_gcode_m500 },
    { 'M', 501U, -1, mc_gcode_m501 },
    { 'M', 502U, -1, mc_gcode_m502 },
    { 'M', 503U, -1, mc_gcode_m503 },
    { 'M', 600U, -1, mc_gcode_m600 },
    { 'M', 701U, -1, mc_gcode_m701 },
    { 'M', 702U, -1, mc_gcode_m702 },
    { 'M', 412U, -1, mc_gcode_m412 },
    { 'M', 24U,  -1, mc_gcode_m24 },
    { 'M', 25U,  -1, mc_gcode_m25 },
    { 'M', 112U, -1, mc_gcode_m112 },
    { 'M', 410U, -1, mc_gcode_m410 },
    { 'M', 999U, -1, mc_gcode_m999 },
    { 'M', 9999U, -1, mc_gcode_m9999 }   // دستور تست بند ۳.۵ (موقت)
};

constexpr uint16_t kTableCount = static_cast<uint16_t>(sizeof(kTable) / sizeof(kTable[0]));


}  // namespace

const GcodeEntry* mc_gcode_find(char letter, uint16_t number, int8_t sub) {
    for (uint16_t i = 0U; i < kTableCount; ++i) {
        const GcodeEntry& e = kTable[i];
        if ((e.letter == letter) && (e.number == number) && (e.sub == sub)) {
            return &e;
        }
    }
    return nullptr;
}

}  // namespace mc
