#pragma once
// کاوش با پراب BLTouch (بند ۶.۶): دستور G30 و آفست پراب (M851). هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "../config/mc_config.h"
#include "gcode_handlers.h"

namespace mc {

constexpr float kProbeMaxMm          = MC_PROBE_MAX_MM;       // سقف مسافت پایین‌رفتن Z (میلی‌متر)
constexpr float kProbeApproachMmS    = MC_PROBE_APPROACH_MM_S;  // سرعت پایین‌رفتن تا برخورد (میلی‌متر بر ثانیه)
constexpr float kProbeReturnMmS      = MC_PROBE_RETURN_MM_S;    // سرعت برگشت به ارتفاع شروع (میلی‌متر بر ثانیه)
constexpr float kProbeOffsetXYMaxMm  = MC_PROBE_OFFSET_XY_LIMIT_MM; // حد قدر مطلق آفست X و Y
constexpr float kProbeOffsetZMaxMm   = MC_PROBE_OFFSET_Z_LIMIT_MM;  // حد قدر مطلق آفست Z

// یک‌بار هنگام راه‌اندازی. به سخت‌افزار دست نمی‌زند.
void mc_probe_init();
// وظیفه زمان‌بند (هر ۱۰۰ میلی‌ثانیه): متغیرهای debug را به‌روز می‌کند.
void mc_probe_poll();

// اجرای G30. تا پایان کار Busy برمی‌گرداند و منتظر نمی‌ماند.
GcodeStatus mc_probe_run(const GcodeCommand& cmd, bool first_call);
// آیا G30 در حال اجراست؟
bool mc_probe_busy();

// آفست پراب (۰ = X ، ۱ = Y ، ۲ = Z) به میلی‌متر. شماره بد ← ۰.
float mc_probe_offset_get(uint8_t axis);
// آیا این مقدار برای این محور مجاز است؟ (شماره محور بد، NaN یا بیرون از حد ← false)
bool mc_probe_offset_valid(uint8_t axis, float mm);
// مقدار را می‌گذارد. false = نامعتبر و چیزی عوض نمی‌شود.
bool mc_probe_offset_set(uint8_t axis, float mm);
// یک دستور رد شد (پارامتر نامعتبر)؛ فقط شمارنده را بالا می‌برد.
void mc_probe_note_refused();

}  // namespace mc
