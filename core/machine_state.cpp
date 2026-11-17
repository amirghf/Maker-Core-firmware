#include "machine_state.h"
#include "motion_queue.h"   // بند ۵.۱۲: برای mc_mq_busy

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_185_motion_fake_busy = 0;   // نوشتنی (تست): عدد 1 یعنی «حرکت در جریان است» (ساختگی)، 0 یعنی ساختگی خاموش. حرکت واقعی جدا حساب می‌شود (بند ۵.۱۲)
}

namespace mc {
namespace {

float g_pos[kAxisCount] = { 0.0f, 0.0f, 0.0f, 0.0f };   // موقعیت محورها (میلی‌متر)

}  // namespace

float mc_position_get(uint8_t axis) {
    if (axis >= kAxisCount) { return 0.0f; }
    return g_pos[axis];
}

void mc_position_set(uint8_t axis, float mm) {
    if (axis >= kAxisCount) { return; }
    g_pos[axis] = mm;
}

bool mc_motion_busy() {
    if (mc_dbg_185_motion_fake_busy != 0U) { return true; }   // مسیر تست (ساختگی)
    return mc_mq_busy();                                       // بند ۵.۱۲: حرکت واقعی (بلوک در صف یا در اجرا)
}

}  // namespace mc
