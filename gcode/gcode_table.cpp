#include "gcode_handlers.h"

namespace mc {
namespace {

// جدول دستورها. برای افزودن دستور جدید: یک فایل در gcode بساز و یک خط به این جدول اضافه کن.
const GcodeEntry kTable[] = {
    // حرف، شماره، زیرکد (-۱ = بدون زیرکد)، تابع
    { 'G', 4U,   -1, mc_gcode_g4 },
    { 'M', 104U, -1, mc_gcode_m104 },
    { 'M', 105U, -1, mc_gcode_m105 },
    { 'M', 106U, -1, mc_gcode_m106 },
    { 'M', 107U, -1, mc_gcode_m107 },
    { 'M', 109U, -1, mc_gcode_m109 },
    { 'M', 110U, -1, mc_gcode_m110 },
    { 'M', 114U, -1, mc_gcode_m114 },
    { 'M', 115U, -1, mc_gcode_m115 },
    { 'M', 140U, -1, mc_gcode_m140 },
    { 'M', 155U, -1, mc_gcode_m155 },
    { 'M', 190U, -1, mc_gcode_m190 },
    { 'M', 400U, -1, mc_gcode_m400 },
    { 'M', 9999U, -1, mc_gcode_m9999 },   // دستور تست بند ۳.۵ (موقت)
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
