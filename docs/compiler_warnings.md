# 🛠️ راهنمای پرچم‌ها و هشدارهای کامپایلر (Compiler Warnings) - MakerCore

این سند پرچم‌های سخت‌گیرانه پیشنهادی برای کامپایلر C++ در محیط STM32CubeIDE و فهرست هشدارهایی که در فایل‌های پروژه برطرف شده‌اند را توضیح می‌دهد.

---

## 🔷 ۱. پرچم‌های کامپایلر پیشنهادی (Recommended Flags)

برای اطمینان از سلامت کد و عدم وجود باگ‌های پنهان، پرچم‌های زیر پیشنهاد می‌شوند:

| پرچم | توضیح |
| :--- | :--- |
| `-Wall` | فعال‌سازی هشدارهای عمومی پایه |
| `-Wextra` | فعال‌سازی هشدارهای تکمیلی (پارامترهای استفاده‌نشده، مقایسه‌های ناهمخوان و ...) |
| `-Wshadow` | اعلام هشدار در صورت تعریف متغیر هم‌نام در scope داخلی که متغیر خارجی را می‌پوشاند |
| `-Wconversion` | اعلام هشدار در صورت تبدیل ضمنی نوع داده که احتمال قطع شدن عدد دارد (مثلاً float به int یا uint32_t به uint8_t) |
| `-Wdouble-promotion` | اعلام هشدار در صورت تبدیل ضمنی float به double در میکروکنترلرهای دارای FPU تک‌دقتی |

---

## 🔷 ۲. مسیر تنظیم پرچم‌ها در STM32CubeIDE

برای فعال‌سازی پرچم‌ها در پروژه:

1. روی اسم پروژه در Explorer راست‌کلیک کرده و **Properties** را انتخاب کنید.
2. مسیر زیر را طی کنید:
   `C/C++ Build` > `Settings` > `MCU GCC Compiler` > `Warnings`
3. همین مسیر را برای کامپایلر C++ نیز طی کنید:
   `C/C++ Build` > `Settings` > `MCU G++ Compiler` > `Warnings`
4. گزینه **Enable All Warnings (-Wall)** و **Enable Extra Warnings (-Wextra)** را تیک بزنید.
5. در بخش **Other warning flags** پرچم‌های `-Wshadow -Wconversion -Wdouble-promotion` را اضافه کنید.

---

## 🔷 ۳. فهرست هشدارهای برطرف‌شده در کد پروژه

| فایل | نوع هشدار | توضیح و نحوه رفع |
| :--- | :--- | :--- |
| `gcode/m999.cpp` | Missing return statement | نبودن دستور `return` در انتهای تابع غیر void (`mc_gcode_m999`)؛ اضافه کردن `return GcodeStatus::Done;`. |
| `gcode/m112.cpp` | Missing return statement | نبودن دستور `return` در انتهای تابع `mc_gcode_m112`؛ اضافه کردن `return GcodeStatus::Done;`. |
| `core/error.cpp` | Unused variable | متغیرهای استفاده‌نشده `tx` و `ty` در بررسی لمس صفحه kill؛ علامت‌گذاری با `(void)tx; (void)ty;`. |
| `core/soft_limits.cpp` | Implicit conversion / type mismatch | cast صریح `static_cast` برای تبدیل میلی‌متر به دهم میلی‌متر و نوع داده‌های انکودر و دیباگ. |
| `core/probe.cpp` | Signed/Unsigned comparison & conversion | cast صریح `lroundf` و `static_cast` در محاسبه میکرومتر و آفست‌های پراب. |
| `core/homing.cpp` | Signed/Unsigned comparison & conversion | cast صریح `static_cast` در بررسی‌های ماسک هومینگ و نقاط امن پراب. |
| `core/endstop_print.cpp` | Unused parameter / explicit cast | بررسی متغیرهای دیباگ و cast صریح برای ماسک اندستاپ‌ها. |
| `core/alert.cpp` | Unused parameter / string bounds | استفاده از cast صریح در تبدیل AlertLevel به MsgType و کنترل طول بافر رشته‌ها. |
| `ui/ui_filchg.cpp` | variable set but not used | حذف متغیر نااستفاده `sensor_ok` در `mc_ui_filchg_poll` و فراخوانی `mc_temp_get_x10` با `(void)`. |

