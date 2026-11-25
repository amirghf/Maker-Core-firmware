#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// متغیرهای دیباگ توزیع‌کننده دستور (تعریف‌شده در gcode_dispatch.cpp)
extern volatile uint32_t mc_dbg_disp_letter; // کد حرف آخرین دستور برداشته‌شده
extern volatile uint32_t mc_dbg_disp_number; // شماره آخرین دستور برداشته‌شده

#ifdef __cplusplus
}

namespace mc {

// هر دور حلقه اصلی (دوره ۰) صدا زده شود.
void mc_dispatch_poll();

// آیا الان یک دستور در حال اجراست؟ (برای M400 و ... در بندهای بعد)
bool mc_dispatch_busy();

// دستوری که الان در توزیع‌کننده در حال اجراست را رها می‌کند.
void mc_dispatch_abort();

}  // namespace mc
#endif
