#pragma once
// صف دستورها: دستورهای تجزیه‌شده (GcodeCommand) به ترتیب دریافت در آن نگه داشته می‌شوند
// تا بعداً (بند ۴.۵) توزیع‌کننده یکی‌یکی آن‌ها را بردارد. حافظه ثابت است.
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "gcode_parser.h"

namespace mc {

constexpr uint16_t kCmdQueueSize = 8;   // باید توان ۲ باشد. ظرفیت واقعی یکی کمتر است (۷ دستور).

// دستور را (کپی کامل) به انتهای صف اضافه می‌کند. false = صف پر است و چیزی اضافه نشد.
bool mc_cmdq_push(const GcodeCommand& cmd);
// قدیمی‌ترین دستور را برمی‌دارد و در out می‌گذارد. false = صف خالی است.
bool mc_cmdq_pop(GcodeCommand& out);
// آیا صف پر است؟ (دستور جدید جا ندارد)
bool mc_cmdq_full();
// آیا صف خالی است؟
bool mc_cmdq_empty();
// تعداد دستورهای داخل صف.
uint16_t mc_cmdq_count();
// ظرفیت واقعی صف (۷).
uint16_t mc_cmdq_capacity();

}  // namespace mc
