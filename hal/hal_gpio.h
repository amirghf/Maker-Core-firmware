#pragma once
// رابط GPIO مستقل از برد. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
namespace mc {

// نام منطقی پین‌ها. ترتیب این فهرست باید دقیقاً با جدول kPins در board_gpio.cpp یکی باشد.
enum class PinId : uint8_t {
    XStep, XDir, XEn,
    YStep, YDir, YEn,
    ZStep, ZDir, ZEn,
    EStep, EDir, EEn,
    HeaterEx, HeaterBed, FanFilament,
    Buzzer, LcdBlk, LcdCs, LcdRs, LcdRes,
    TouchCs, SdCs, FlashCs, WifiCs,
    KeyStart, KeyStop, EncBtn,
    LimitXMin, LimitYMin, LimitZMin,
    FilamentSensor, BltouchKey, SdDet, TouchIrq,
    Count  // همیشه آخر؛ تعداد پین‌ها
};

enum class PinMode : uint8_t {
    Output,
    Input,          // بدون پول‌آپ و پایین‌کش داخلی
    InputPullUp,
    InputPullDown
};

// حالت پین را عوض می‌کند. هنگام تبدیل به خروجی، سطح قبلی حفظ می‌شود (بدون پرش).
void mc_gpio_set_mode(PinId pin, PinMode mode);
// فقط اگر پین خروجی باشد می‌نویسد؛ روی ورودی هیچ کاری نمی‌کند.
void mc_gpio_write(PinId pin, bool high);
// سطح فیزیکی پین را می‌خواند. شماره نامعتبر = false.
bool mc_gpio_read(PinId pin);
// سطح خروجی را برعکس می‌کند (فقط برای پین خروجی).
void mc_gpio_toggle(PinId pin);
// تست خودکار لایه GPIO. true = سالم.
bool mc_gpio_selftest();

}  // namespace mc

extern "C" {
#endif

// پل برای کد C (main.c)
int  mc_gpio_selftest_c(void);     // نتیجه را در mc_gpio_selftest_ok هم می‌گذارد
void mc_gpio_debug_poll_c(void);   // وضعیت کلیدها را در متغیرهای دیباگ به‌روز می‌کند

#ifdef __cplusplus
}
#endif
