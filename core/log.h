#pragma once
// سیستم لاگ. هیچ هدر ST اینجا include نمی‌شود.
// فقط از وظیفه‌های حلقه اصلی صدا زده شود (نه از وقفه، نه از mc_kill).
#include <stdint.h>

namespace mc {

// سطح لاگ. عدد کمتر یعنی مهم‌تر.
enum class LogLevel : uint8_t {
    Error = 0,
    Warn  = 1,
    Info  = 2,
    Debug = 3
};

// یک خط لاگ در صف می‌گذارد. اگر سطح آن از سطح فعلی بالاتر (کم‌اهمیت‌تر) باشد، نادیده گرفته می‌شود.
// متن بیش از حدِ جا بریده می‌شود. اگر صف پر باشد خط دور ریخته و شمرده می‌شود.
void mc_log(LogLevel level, const char* text);

// مثل mc_log ولی در انتها « = عدد » هم اضافه می‌کند.
void mc_log_value(LogLevel level, const char* text, uint32_t value);

// سطح فعلی لاگ را تنظیم یا می‌خواند. پیش‌فرض Info است.
void mc_log_set_level(LogLevel level);
LogLevel mc_log_get_level();

// یک خط از صف را برای کامپیوتر می‌فرستد (فقط اگر پورت باز باشد). به‌صورت یک وظیفه صدا زده شود.
void mc_log_flush();

}  // namespace mc
