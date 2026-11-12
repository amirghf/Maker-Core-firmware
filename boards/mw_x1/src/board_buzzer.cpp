// پیاده‌سازی بیزر برای برد MW-X1 (بیزر Active ، سطح فعال HIGH روی PC5).
// فقط پوشه boards مجاز به include کردن هدرهای ST است (HAL_Delay از pins.h می‌آید).
#include "hal_buzzer.h"
#include "hal_gpio.h"
#include "pins.h"

namespace mc {
namespace {

// بیزر را روشن (true) یا خاموش (false) می‌کند.
inline void buzzer_set(bool on) {
    mc_gpio_write(PinId::Buzzer, on);
}

// یک بوق: ms_on میلی‌ثانیه روشن، بعد ms_off میلی‌ثانیه خاموش.
void pulse(uint32_t ms_on, uint32_t ms_off) {
    buzzer_set(true);
    HAL_Delay(ms_on);
    buzzer_set(false);
    HAL_Delay(ms_off);
}

// چند بوق پشت سر هم.
void repeat(uint8_t count, uint32_t ms_on, uint32_t ms_off) {
    for (uint8_t i = 0; i < count; ++i) {
        pulse(ms_on, ms_off);
    }
}

// صبر می‌کند تا کلید رها شود (کلیدها هنگام فشردن LOW می‌شوند).
void wait_release(PinId key) {
    while (!mc_gpio_read(key)) {
        HAL_Delay(10);
    }
    HAL_Delay(30);  // ضدلرزش بعد از رها شدن
}

// آیا کلید واقعاً فشرده است؟ (فشردن = LOW). ۳۰ میلی‌ثانیه صبر می‌کنیم تا لرزش تمام شود.
bool key_pressed(PinId key) {
    if (mc_gpio_read(key)) { return false; }
    HAL_Delay(30);
    return !mc_gpio_read(key);
}

}  // namespace

void mc_buzzer_off() {
    buzzer_set(false);
}

void mc_buzzer_beep(uint32_t ms) {
    buzzer_set(true);
    HAL_Delay(ms);
    buzzer_set(false);
}

void mc_buzzer_play(Sound sound) {
    switch (sound) {
        case Sound::Boot:    mc_buzzer_beep(200);   break;
        case Sound::Ok:      repeat(2, 100, 100);   break;
        case Sound::Warning: mc_buzzer_beep(600);   break;
        case Sound::Error:   repeat(3, 150, 150);   break;
    }
    buzzer_set(false);  // ایمنی: همیشه آخرش خاموش
}

}  // namespace mc

static_assert(MC_SOUND_BOOT    == static_cast<int>(mc::Sound::Boot),    "MC_SOUND_BOOT با enum یکی نیست");
static_assert(MC_SOUND_OK      == static_cast<int>(mc::Sound::Ok),      "MC_SOUND_OK با enum یکی نیست");
static_assert(MC_SOUND_WARNING == static_cast<int>(mc::Sound::Warning), "MC_SOUND_WARNING با enum یکی نیست");
static_assert(MC_SOUND_ERROR   == static_cast<int>(mc::Sound::Error),   "MC_SOUND_ERROR با enum یکی نیست");

extern "C" void mc_buzzer_play_c(int sound_id) {
    if (sound_id < MC_SOUND_BOOT || sound_id > MC_SOUND_ERROR) { return; }
    mc::mc_buzzer_play(static_cast<mc::Sound>(sound_id));
}

extern "C" void mc_buzzer_test_poll_c(void) {
    using namespace mc;
    if (key_pressed(PinId::KeyStart)) {
        mc_buzzer_play(Sound::Ok);
        wait_release(PinId::KeyStart);
    } else if (key_pressed(PinId::KeyStop)) {
        mc_buzzer_play(Sound::Error);
        wait_release(PinId::KeyStop);
    } else if (key_pressed(PinId::EncBtn)) {
        mc_buzzer_play(Sound::Warning);
        wait_release(PinId::EncBtn);
    }
}
