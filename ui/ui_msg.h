#pragma once
// صفحه پیام‌های Error، Warning و Info (بند ۱۰.۱).
// هیچ هدر ST اینجا include نمی‌شود.
#include <stdint.h>
#include "../config/mc_config.h"

#ifdef __cplusplus
extern "C" {
#endif

extern volatile uint32_t mc_dbg_464_msg_force;         // نوشتنی: 0 = بی‌اثر. 1 = Error نمونه، 2 = Warning نمونه، 3 = Info نمونه. UI مقدار را 0 می‌کند.
extern volatile uint32_t mc_dbg_465_msg_sample;        // نوشتنی: شماره نمونه متن 0 تا 3 (پیش‌فرض 0).
extern volatile uint32_t mc_dbg_466_msg_shown;         // خواندنی: 1 = صفحه پیام الان نمایش داده می‌شود، 0 = نه.
extern volatile uint32_t mc_dbg_467_msg_type;          // خواندنی: نوع پیام فعلی: 0 = هیچ، 1 = Error، 2 = Warning، 3 = Info.
extern volatile uint32_t mc_dbg_468_msg_show_cnt;      // خواندنی: تعداد دفعاتی که پیام نمایش داده شد.
extern volatile uint32_t mc_dbg_469_msg_close_cnt;     // خواندنی: تعداد دفعاتی که پیام با OK یا انکودر بسته شد.
extern volatile uint32_t mc_dbg_470_msg_close_src;     // خواندنی: منبع آخرین بستن: 0 = هنوز، 1 = دکمه انکودر، 2 = لمس دکمه OK.
extern volatile uint32_t mc_dbg_471_msg_refused;       // خواندنی: تعداد دفعاتی که درخواست نمایش پیام رد شد.
extern volatile uint32_t mc_dbg_472_msg_refuse_reason; // خواندنی: علت آخرین رد شدن: 0 = هنوز، 1 = صفحه فعال 0 نبود یا UI آماده نبود، 2 = نوع/نمونه نامعتبر.

#ifdef __cplusplus
}

namespace mc {

enum class MsgType : uint8_t { None = 0, Error = 1, Warning = 2, Info = 3 };
enum class MsgUiResult : uint8_t { Stay = 0, Closed = 1 };

// پیام را آماده نمایش می‌کند. متن‌ها را داخل بافرهای ثابت داخلی کپی می‌کند (بدون حافظه پویا).
void mc_ui_msg_show(MsgType type, const char* text, const char* detail);
// گام‌های نقاشی را از اول شروع می‌کند و ورودی‌های قدیمی لمس را دور می‌ریزد.
void mc_ui_msg_open();
// هر دور mc_ui_poll وقتی صفحه ۱۲ فعال است یک بار صدا زده می‌شود.
MsgUiResult mc_ui_msg_poll();
// پیام را بدون هیچ خطا لغو می‌کند.
void mc_ui_msg_abort();
// نوع پیام فعلی (None اگر پیامی نیست).
MsgType mc_ui_msg_type();
// مقدار اولیه متغیرها (از mc_ui_init صدا زده شود).
void mc_ui_msg_init();
// بخش متغیر debug تست. هر دور mc_ui_poll صدا زده می‌شود.
void mc_ui_msg_test_poll();
// آیا پیام آماده باز شدن روی صفحه اصلی است؟
bool mc_ui_msg_pending();

}  // namespace mc
#endif
