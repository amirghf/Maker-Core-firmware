#pragma once
// درایور BLTouch (بند ۶.۵). سروو را از طریق hal_servo.h فرمان می‌دهد و منتظر نمی‌ماند (بدون حلقه طولانی).
// سیگنال پراب روی ورودی Z-min (PC4) می‌آید. در این بند فقط سطح خام آن نمایش داده می‌شود.
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

namespace mc {

// فرمان‌های BLTouch. ترتیب باید با جدول kCmds در bltouch.cpp یکی باشد.
enum class BltouchCmd : uint8_t {
    Deploy       = 0,   // بیرون آوردن پین (زاویه ۱۰)
    Stow         = 1,   // جمع کردن پین (زاویه ۹۰)
    SelfTest     = 2,   // تست خود پراب (زاویه ۱۲۰)
    AlarmRelease = 3    // آزاد کردن آلارم (زاویه ۱۶۰)
};

// وضعیت پین بعد از آخرین فرمان کامل‌شده.
enum class BltouchPin : uint8_t {
    Unknown  = 0,   // نامعلوم (بعد از روشن‌شدن، حین فرمان، بعد از SelfTest یا AlarmRelease)
    Deployed = 1,   // بیرون
    Stowed   = 2    // جمع
};

constexpr uint32_t kBltouchPeriodMs         = 10U;     // دوره وظیفه زمان‌بند
constexpr uint32_t kBltouchHoldMs           = 500U;    // مدت ارسال پالس برای هر فرمان
constexpr uint32_t kBltouchSettleMs         = 300U;    // صبر بعد از Deploy و Stow
constexpr uint32_t kBltouchSelfTestSettleMs = 3000U;   // صبر بعد از SelfTest
constexpr uint32_t kBltouchAlarmSettleMs    = 500U;    // صبر بعد از AlarmRelease

// یک‌بار هنگام راه‌اندازی. به سخت‌افزار دست نمی‌زند.
void mc_bltouch_init();
// وظیفه زمان‌بند (هر kBltouchPeriodMs): مرحله پالس و صبر را جلو می‌برد و متغیرهای debug را به‌روز می‌کند.
void mc_bltouch_poll();

// فرمان را شروع می‌کند. false = رد شد (مشغول، حالت kill، فرمان نامعتبر یا خطای سروو) و شمارنده رد شدن بالا می‌رود.
bool mc_bltouch_start(BltouchCmd cmd);
// آیا فرمانی (پالس یا صبر) در جریان است؟
bool mc_bltouch_busy();
// وضعیت پین بعد از آخرین فرمان کامل‌شده.
BltouchPin mc_bltouch_pin_state();
// سطح خام پین سیگنال پراب (HIGH = true). همان خواندن خام Z-min است و وارونه نمی‌شود.
bool mc_bltouch_probe_raw();
// زاویه (۱۰ یا ۹۰ یا ۱۲۰ یا ۱۶۰) را به فرمان تبدیل می‌کند. false = زاویه مجاز نیست و out تغییر نمی‌کند.
bool mc_bltouch_cmd_from_angle(uint32_t angle, BltouchCmd& out);
// یک دستور G-code رد شد (پارامتر نامعتبر)؛ فقط شمارنده را بالا می‌برد.
void mc_bltouch_note_refused();

}  // namespace mc
