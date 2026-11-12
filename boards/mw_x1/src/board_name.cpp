// نام برد MW-X1 برای هسته (هسته به board_info.h دسترسی ندارد).
#include "hal_board.h"
#include "board_info.h"

namespace mc {

const char* mc_board_name() {
    return MC_BOARD_NAME;
}

}  // namespace mc
