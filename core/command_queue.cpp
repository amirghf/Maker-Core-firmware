#include "command_queue.h"
#include "ring_buffer.h"

// متغیرهای دیباگ؛ در CubeIDE با Live Expressions دیده می‌شوند.
extern "C" {
volatile uint32_t mc_dbg_cq_count  = 0;   // تعداد دستورهای داخل صف
volatile uint32_t mc_dbg_cq_pushed = 0;   // کل دستورهایی که وارد صف شدند
volatile uint32_t mc_dbg_cq_popped = 0;   // کل دستورهایی که از صف برداشته شدند
}

namespace mc {
namespace {

RingBuffer<GcodeCommand, kCmdQueueSize> g_queue;   // حافظه ثابت (حدود ۱٫۹ کیلوبایت)

}  // namespace

bool mc_cmdq_push(const GcodeCommand& cmd) {
    if (!g_queue.push(cmd)) { return false; }
    mc_dbg_cq_pushed = mc_dbg_cq_pushed + 1U;
    mc_dbg_cq_count = g_queue.size();
    return true;
}

bool mc_cmdq_pop(GcodeCommand& out) {
    if (!g_queue.pop(out)) { return false; }
    mc_dbg_cq_popped = mc_dbg_cq_popped + 1U;
    mc_dbg_cq_count = g_queue.size();
    return true;
}

bool mc_cmdq_full() {
    return g_queue.full();
}

bool mc_cmdq_empty() {
    return g_queue.empty();
}

uint16_t mc_cmdq_count() {
    return g_queue.size();
}

uint16_t mc_cmdq_capacity() {
    return g_queue.capacity();
}

}  // namespace mc
