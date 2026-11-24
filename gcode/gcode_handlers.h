#pragma once
// نوع تابع‌های دستور و جدول دستورها. هر دستور G-code در یک فایل جدا در پوشه gcode است.
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "gcode_parser.h"

namespace mc {

// نتیجه اجرای یک دستور
enum class GcodeStatus : uint8_t {
    Done        = 0,   // دستور تمام شد
    Busy        = 1,   // هنوز کار دارد؛ دفعه بعد دوباره صدا زده شود و دستور بعدی برداشته نشود
    Error       = 2,   // شمرده و لاگ می‌شود و برای کامپیوتر Error:... و ok فرستاده می‌شود (بند ۳.۶)
    DoneReplied = 3    // دستور تمام شد و خودش ok را (همراه متن) فرستاده؛ توزیع‌کننده ok جدا نمی‌فرستد
};

// تابع یک دستور. first_call فقط بار اول (برای هر دستور) true است؛ بعد از آن اگر Busy برگردانده
// باشد دوباره با همان cmd و first_call = false صدا زده می‌شود.
// تابع نباید منتظر بماند (حلقه نزند)؛ برای انتظار باید Busy برگرداند.
using GcodeHandler = GcodeStatus (*)(const GcodeCommand& cmd, bool first_call);

struct GcodeEntry {
    char         letter;    // 'G' یا 'M' یا 'T'
    uint16_t     number;    // مثلاً ۲۸
    int8_t       sub;       // زیرکد (G38.2 ← ۲)، یا -۱ اگر زیرکد ندارد
    GcodeHandler handler;
};

// دستور را در جدول (gcode_table.cpp) پیدا می‌کند. nullptr = دستور ناشناس.
const GcodeEntry* mc_gcode_find(char letter, uint16_t number, int8_t sub);

GcodeStatus mc_gcode_g4(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m104(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m105(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m106(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m107(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m109(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m110(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m114(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m115(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m119(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m140(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m155(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m190(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m303(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m400(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_g90(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_g91(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_g92(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_g28(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m82(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m83(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m92(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m302(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_g0(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_g1(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m17(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m18(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m84(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m203(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m204(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m205(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m211(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m280(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m401(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m402(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_g30(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m851(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m500(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m501(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m502(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m503(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m24(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m25(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m108(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m410(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m600(const GcodeCommand& cmd, bool first_call);
GcodeStatus mc_gcode_m999(const GcodeCommand& cmd, bool first_call);


// دستور تست این بند: M9999 (فایل m9999.cpp)
GcodeStatus mc_gcode_m9999(const GcodeCommand& cmd, bool first_call);

}  // namespace mc
