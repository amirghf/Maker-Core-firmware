#pragma once
// PWM نرم‌افزاری هیتر اکسترودر، هیتر بستر و فن (بند ۴.۳). هیچ هدر ST اینجا include نمی‌شود.
// توان هر خروجی ۰ تا ۲۵۵ است. PWM داخل وقفه ۱۰۰ میکروثانیه‌ای (هوک زمان) ساخته می‌شود.
#include <stdint.h>
namespace mc {
enum class OutputId : uint8_t {
    HeaterEx  = 0,   // هیتر اکسترودر
    HeaterBed = 1,   // هیتر بستر
    Fan       = 2,   // فن
    Count     = 3
};
// طول یک دوره PWM بر حسب تیک ۱۰۰ میکروثانیه‌ای
constexpr uint16_t kHeaterExCycleTicks  = 1000;    // ۱۰۰ میلی‌ثانیه
constexpr uint16_t kHeaterBedCycleTicks = 10000;   // ۱ ثانیه
constexpr uint16_t kFanCycleTicks       = 100;     // ۱۰ میلی‌ثانیه (۱۰۰ هرتز)
// هوک را ثبت می‌کند و همه خروجی‌ها را صفر می‌کند. یک‌بار بعد از mc_time_init صدا زده شود.
void mc_output_init();
// توان یک خروجی را می‌گذارد (۰ تا ۲۵۵). شماره نامعتبر نادیده گرفته می‌شود.
void mc_output_set(OutputId id, uint8_t power);
// آخرین توانی که گذاشته شده (شماره نامعتبر ← ۰).
uint8_t mc_output_get(OutputId id);
// همه خروجی‌ها را صفر می‌کند.
void mc_output_all_off();
// وظیفه زمان‌بند (هر ۱۰۰ میلی‌ثانیه): متغیرهای debug را به‌روز می‌کند و (موقت) توان فن را از متغیرهای debug می‌گیرد. هیتر اکسترودر با PID (بند ۴.۴) و هیتر بستر با heater_bed (بند ۴.۵) است.
void mc_output_poll();
}  // namespace mc
