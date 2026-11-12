#include "machine_state.h"

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_motion_fake_busy = 0;   // برای تست: نوشتن عدد ۱ یعنی «حرکت در جریان است»، ۰ یعنی تمام شد
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
    return mc_dbg_motion_fake_busy != 0U;
}

}  // namespace mc
