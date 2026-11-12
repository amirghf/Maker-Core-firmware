#include "gcode_handlers.h"

namespace mc {
namespace {

// جدول دستورها. برای افزودن دستور جدید: یک فایل در gcode بساز و یک خط به این جدول اضافه کن.
const GcodeEntry kTable[] = {
    // حرف، شماره، زیرکد (-۱ = بدون زیرکد)، تابع
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
