#include "gcode_parser.h"

namespace mc {
namespace {

inline bool is_space(char c)  { return (c == ' ') || (c == '\t'); }
inline bool is_digit(char c)  { return (c >= '0') && (c <= '9'); }
inline bool is_letter(char c) { return ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')); }
inline char upper(char c)     { return ((c >= 'a') && (c <= 'z')) ? static_cast<char>(c - 32) : c; }

// دستورهایی که بعدشان متن می‌آید (نام فایل یا پیام) و پارامتر حرفی ندارند.
bool is_string_command(char letter, uint16_t number) {
    if (letter != 'M') { return false; }
    return (number == 23U) || (number == 28U) || (number == 30U) ||
           (number == 32U) || (number == 117U) || (number == 118U);
}

// عدد اعشاری ساده (علامت اختیاری، رقم، نقطه). توان ۱۰ (e) پشتیبانی نمی‌شود.
// p روی اولین کاراکتر عدد است و بعد از خواندن روی اولین کاراکتر بعد از عدد می‌ایستد.
bool parse_number(const char*& p, float& out) {
    static const float kPow10[10] = { 1.0f, 10.0f, 100.0f, 1000.0f, 10000.0f,
                                      100000.0f, 1000000.0f, 10000000.0f, 100000000.0f, 1000000000.0f };
    bool neg = false;
    if ((*p == '+') || (*p == '-')) {
        neg = (*p == '-');
        ++p;
    }
    uint32_t mant = 0U;
    uint8_t  frac = 0U;
    bool     any  = false;
    bool     dot  = false;
    for (;;) {
        const char c = *p;
        if (is_digit(c)) {
            any = true;
            if (mant <= 99999999U) {
                mant = (mant * 10U) + static_cast<uint32_t>(c - '0');
                if (dot) { ++frac; }
            } else if (!dot) {
                return false;   // بخش صحیح بیش از ۹ رقم
            }                   // رقم‌های اعشاری اضافه نادیده گرفته می‌شوند
        } else if ((c == '.') && !dot) {
            dot = true;
        } else {
            break;
        }
        ++p;
    }
    if (!any) { return false; }
    const float v = static_cast<float>(mant) / kPow10[frac];
    out = neg ? -v : v;
    return true;
}

const GcodeParam* find_param(const GcodeCommand& cmd, char letter) {
    const char l = upper(letter);
    for (uint8_t i = 0U; i < cmd.param_count; ++i) {
        if (cmd.params[i].letter == l) { return &cmd.params[i]; }
    }
    return nullptr;
}

}  // namespace

GcodeResult mc_gcode_parse(const char* line, GcodeCommand& out) {
    out.letter      = '\0';
    out.number      = 0U;
    out.sub         = -1;
    out.param_count = 0U;
    out.args_offset = 0U;
    out.text[0]     = '\0';
    if (line == nullptr) { return GcodeResult::Empty; }

    // کپی خط تا قبل از کامنت (;) یا پایان
    uint16_t n = 0U;
    while ((line[n] != '\0') && (line[n] != ';') && (n < kLineMaxChars)) {
        out.text[n] = line[n];
        ++n;
    }
    out.text[n] = '\0';
    while ((n > 0U) && is_space(out.text[n - 1U])) {   // فاصله‌های انتها
        --n;
        out.text[n] = '\0';
    }

    const char* p = out.text;
    while (is_space(*p)) { ++p; }
    if (*p == '\0') { return GcodeResult::Empty; }

    // کلمه دستور: حرف + عدد + (نقطه و یک رقم اختیاری)
    const char c0 = upper(*p);
    if ((c0 != 'G') && (c0 != 'M') && (c0 != 'T')) { return GcodeResult::BadCommand; }
    ++p;
    if (!is_digit(*p)) { return GcodeResult::BadCommand; }
    uint32_t num = 0U;
    while (is_digit(*p)) {
        num = (num * 10U) + static_cast<uint32_t>(*p - '0');
        if (num > 9999U) { return GcodeResult::BadCommand; }
        ++p;
    }
    out.letter = c0;
    out.number = static_cast<uint16_t>(num);
    if (*p == '.') {
        ++p;
        if (!is_digit(*p)) { return GcodeResult::BadCommand; }
        out.sub = static_cast<int8_t>(*p - '0');
        ++p;
        if (is_digit(*p)) { return GcodeResult::BadCommand; }   // فقط یک رقم بعد از نقطه
    }

    // متن بعد از کلمه دستور
    const char* q = p;
    while (is_space(*q)) { ++q; }
    out.args_offset = static_cast<uint8_t>(q - out.text);
    if (is_string_command(out.letter, out.number)) { return GcodeResult::Ok; }

    // پارامترها
    for (;;) {
        while (is_space(*p)) { ++p; }
        if (*p == '\0') { break; }
        if (!is_letter(*p)) { return GcodeResult::BadParam; }

        GcodeParam par;
        par.letter    = upper(*p);
        par.has_value = false;
        par.value     = 0.0f;
        ++p;
        const char c = *p;
        if (is_digit(c) || (c == '+') || (c == '-') || (c == '.')) {
            if (!parse_number(p, par.value)) { return GcodeResult::BadParam; }
            par.has_value = true;
        }

        if (find_param(out, par.letter) == nullptr) {   // تکراری باشد، اولی می‌ماند
            if (out.param_count >= kGcodeMaxParams) { return GcodeResult::TooManyParams; }
            out.params[out.param_count] = par;
            out.param_count = static_cast<uint8_t>(out.param_count + 1U);
        }
    }
    return GcodeResult::Ok;
}

bool mc_gcode_has(const GcodeCommand& cmd, char letter) {
    return find_param(cmd, letter) != nullptr;
}

bool mc_gcode_has_value(const GcodeCommand& cmd, char letter) {
    const GcodeParam* p = find_param(cmd, letter);
    return (p != nullptr) && p->has_value;
}

float mc_gcode_value(const GcodeCommand& cmd, char letter, float def) {
    const GcodeParam* p = find_param(cmd, letter);
    return ((p != nullptr) && p->has_value) ? p->value : def;
}

int32_t mc_gcode_int(const GcodeCommand& cmd, char letter, int32_t def) {
    const GcodeParam* p = find_param(cmd, letter);
    if ((p == nullptr) || !p->has_value) { return def; }
    const float f = p->value;
    return static_cast<int32_t>((f >= 0.0f) ? (f + 0.5f) : (f - 0.5f));
}

const char* mc_gcode_args(const GcodeCommand& cmd) {
    return cmd.text + cmd.args_offset;
}

}  // namespace mc
