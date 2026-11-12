#include "text_format.h"

namespace mc {
namespace {

uint16_t put_char(char* dst, uint16_t cap, uint16_t pos, char c) {
    if ((dst == nullptr) || (cap < 2U)) { return pos; }
    if (pos > static_cast<uint16_t>(cap - 1U)) { pos = static_cast<uint16_t>(cap - 1U); }
    if (pos < static_cast<uint16_t>(cap - 1U)) {
        dst[pos] = c;
        pos = static_cast<uint16_t>(pos + 1U);
    }
    dst[pos] = '\0';
    return pos;
}

}  // namespace

uint16_t mc_fmt_text(char* dst, uint16_t cap, uint16_t pos, const char* src) {
    if (src == nullptr) { return pos; }
    while (*src != '\0') {
        pos = put_char(dst, cap, pos, *src);
        src = src + 1;
    }
    return pos;
}

uint16_t mc_fmt_uint(char* dst, uint16_t cap, uint16_t pos, uint32_t value) {
    char digits[10];
    uint8_t d = 0U;
    do {
        digits[d] = static_cast<char>('0' + (value % 10U));
        d = static_cast<uint8_t>(d + 1U);
        value /= 10U;
    } while ((value > 0U) && (d < 10U));
    while (d > 0U) {
        d = static_cast<uint8_t>(d - 1U);
        pos = put_char(dst, cap, pos, digits[d]);
    }
    return pos;
}

uint16_t mc_fmt_float(char* dst, uint16_t cap, uint16_t pos, float value, uint8_t decimals) {
    if (decimals > 3U) { decimals = 3U; }
    if (!(value == value)) { return mc_fmt_text(dst, cap, pos, "nan"); }   // NaN

    bool neg = false;
    if (value < 0.0f) {
        neg = true;
        value = -value;
    }
    if (value > 999999.0f) { value = 999999.0f; }

    uint32_t scale = 1U;
    for (uint8_t i = 0U; i < decimals; ++i) { scale *= 10U; }

    const uint32_t scaled = static_cast<uint32_t>((value * static_cast<float>(scale)) + 0.5f);
    const uint32_t int_part = scaled / scale;
    const uint32_t frac = scaled % scale;

    if (neg && (scaled != 0U)) { pos = put_char(dst, cap, pos, '-'); }
    pos = mc_fmt_uint(dst, cap, pos, int_part);
    if (decimals > 0U) {
        pos = put_char(dst, cap, pos, '.');
        uint32_t div = scale / 10U;
        for (uint8_t i = 0U; i < decimals; ++i) {
            pos = put_char(dst, cap, pos, static_cast<char>('0' + ((frac / div) % 10U)));
            div /= 10U;
        }
    }
    return pos;
}

}  // namespace mc
