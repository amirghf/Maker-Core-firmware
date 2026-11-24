#pragma once
// صفحه تعویض فیلامنت (بند ۹.۱).
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>

namespace mc {

enum class FilChgUiResult : uint8_t {
    Stay = 0,
    Done = 1
};

void mc_ui_filchg_open();
FilChgUiResult mc_ui_filchg_poll();

}  // namespace mc
