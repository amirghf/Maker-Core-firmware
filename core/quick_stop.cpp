// STOP: حرکت‌ها و صف قطع می‌شود، هیتر اکسترودر و بستر و فن خاموش می‌شوند و موتورها غیرفعال می‌شوند (بند ۸.۱۶.۵). هیچ هدر ST اینجا include نمی‌شود.
#include "quick_stop.h"
#include "step_gen.h"
#include "axis_move.h"
#include "motion_queue.h"
#include "planner.h"
#include "command_queue.h"
#include "error.h"
#include "homing.h"
#include "probe.h"
#include "temp_wait.h"
#include "pid_autotune.h"
#include "heater_ex.h"
#include "heater_bed.h"
#include "output_pwm.h"
#include "stepper_out.h"
#include "../gcode/gcode_dispatch.h"

namespace mc {

void mc_quick_stop() {
    if (mc_is_killed()) { return; }

    // ۱) متوقف‌کردن تولیدکننده پالس و حرکت جاری
    mc_stepgen_stop();
    mc_axis_move_abort();

    // ۲) پاک‌کردن صف‌های حرکت و برنامه‌ریز
    mc_mq_clear();
    mc_plan_clear();

    // ۳) پاک‌کردن صف دستورها و رها کردن دستور در حال اجرا
    mc_cmdq_clear();
    mc_dispatch_abort();

    // ۴) بازگرداندن وضعیت ماژول‌های دستورات طولانی به حالت اولیه/بیکار
    mc_homing_init();
    mc_probe_init();
    mc_temp_wait_abort();
    mc_autotune_release();

    // ۵) خاموش‌کردن هیترها و فن و غیرفعال‌کردن موتورها
    (void)mc_heater_ex_set_target_x10(0);
    (void)mc_heater_bed_set_target_x10(0);
    mc_fan_set(0);
    mc_motor_disable_all();
}

}  // namespace mc
