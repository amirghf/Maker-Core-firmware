#pragma once
// درایور انکودر چرخشی و دکمه آن (بند ۸.۸).
// فقط از هدرهای hal استفاده می‌کند. هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "../config/mc_config.h"

namespace mc {

// راه‌اندازی ماژول انکودر
void mc_encoder_init();

// وظیفه زمان‌بند (هر ۱ میلی‌ثانیه): کدگشایی کوادراتور، ضدلرزش دکمه و تولید بوق
void mc_encoder_poll();

// دریافت کلیک‌های چرخش جمع‌شده از آخرین فراخوانی (با علامت، منفی یا مثبت) و صفر کردن بافر
int32_t mc_encoder_take_clicks();

// آیا دکمه از آخرین فراخوانی فشرده شده است؟ (یک‌بار مصرف، بافر صفر می‌شود)
bool mc_encoder_take_press();

// آیا دکمه الان پایین (فشرده) است؟
bool mc_encoder_is_pressed();

}  // namespace mc
