#pragma once
// رابط بیزر مستقل از برد. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

#ifdef __cplusplus
namespace mc {

// کدهای صوتی. اگر مقدار عددی عوض شود باید ماکروهای MC_SOUND_* پایین فایل هم عوض شوند.
enum class Sound : uint8_t {
    Boot    = 0,  // برد سالم بالا آمد
    Ok      = 1,  // تأیید
    Warning = 2,  // هشدار
    Error   = 3   // خطا
};

// بیزر را فوراً خاموش می‌کند.
void mc_buzzer_off();
// بیزر را برای مدت داده‌شده (میلی‌ثانیه) روشن می‌کند و بعد خاموش می‌کند (مسدودکننده).
void mc_buzzer_beep(uint32_t ms);
// یک کد صوتی کامل را پخش می‌کند (مسدودکننده) و در آخر بیزر را خاموش می‌گذارد.
void mc_buzzer_play(Sound sound);

}  // namespace mc

extern "C" {
#endif

// شماره کدهای صوتی برای کد C (main.c). باید با enum class Sound یکی باشد.
#define MC_SOUND_BOOT    0
#define MC_SOUND_OK      1
#define MC_SOUND_WARNING 2
#define MC_SOUND_ERROR   3

// پل برای کد C. شماره نامعتبر = هیچ کاری نمی‌کند.
void mc_buzzer_play_c(int sound_id);
// تست روی برد: START = صدای Ok ، STOP = صدای Error ، دکمه انکودر = صدای Warning
void mc_buzzer_test_poll_c(void);

#ifdef __cplusplus
}
#endif
