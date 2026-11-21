#pragma once
// تنظیمات دستگاه MakerCore (مثل Configuration.h در مارلین).
// همه مقدارهای قابل تنظیم دستگاه اینجا هستند. بعد از هر تغییر باید دوباره بیلد کرد.
// این فایل فقط عدد و انتخاب دارد و هیچ include ای ندارد.
// (انتخاب برد در بند ۱۱.۲ به همین فایل اضافه می‌شود.)
// نسخه و نام فرمویر در mc_version.h است.

// ===================== ۱. مکانیک (بند ۵.۸) =====================
// گام بر میلی‌متر هر محور. با خط‌کش اندازه‌گیری شده (بند ۶.۷):
// X برابر 40، Y حدود 393 (خطای حدود ±8)، Z برابر 400 (با Marlin قبلی دستگاه و تست خط‌کش تأیید شد).
// با دستور M92 هم می‌شود موقتی عوض کرد (تا ریست).
#define MC_STEPS_PER_MM_X   40.0f
#define MC_STEPS_PER_MM_Y   393.0f
#define MC_STEPS_PER_MM_Z   400.0f
#define MC_STEPS_PER_MM_E   100.0f

// ===================== ۲. حدود نرم‌افزاری (بند ۵.۱۰) =====================
// اندازه مسیر قابل استفاده بعد از هومینگ (میلی‌متر).
#define MC_SOFT_MIN_MM      0.0f
#define MC_SOFT_MAX_X_MM    240.0f
#define MC_SOFT_MAX_Y_MM    120.0f
#define MC_SOFT_MAX_Z_MM    200.0f

// ===================== ۳. سرعت و شتاب (بند ۵.۸ و ۵.۱۱) =====================
// حداکثر سرعت هر محور (میلی‌متر بر ثانیه). Y عمداً 5 است (برابر حدود 2000 گام بر ثانیه)؛
// بعد از تست مکانیک می‌شود زیادش کرد.
#define MC_MAX_FEED_X_MM_S      20.0f
#define MC_MAX_FEED_Y_MM_S      5.0f
#define MC_MAX_FEED_Z_MM_S      4.0f
#define MC_MAX_FEED_E_MM_S      10.0f
// شتاب (میلی‌متر بر ثانیه مجذور)
#define MC_ACCEL_PRINT_MM_S2    100.0f
#define MC_ACCEL_RETRACT_MM_S2  100.0f
#define MC_ACCEL_TRAVEL_MM_S2   100.0f
// حداقل سرعت (M205 S) و فید پیش‌فرض G1 (میلی‌متر بر دقیقه)
#define MC_MIN_FEED_MM_S        1.0f
#define MC_DEFAULT_FEED_MM_MIN  1500.0f
// انحراف گوشه برنامه‌ریز (میکرومتر)
#define MC_JUNCTION_DEVIATION_UM 50U

// ===================== ۴. هومینگ (بند ۶.۳ و ۶.۷) =====================
// سقف مسافت جست‌وجوی سوئیچ در مرحله سریع (میلی‌متر)
#define MC_HOME_MAX_X_MM        260.0f
#define MC_HOME_MAX_Y_MM        130.0f
#define MC_HOME_MAX_Z_MM        210.0f
// سرعت مرحله سریع (میلی‌متر بر ثانیه). سقف عملی هر محور 2000 گام بر ثانیه است.
#define MC_HOME_FAST_X_MM_S     10.0f
#define MC_HOME_FAST_Y_MM_S     4.0f
#define MC_HOME_FAST_Z_MM_S     2.0f
// سرعت مرحله آهسته و عقب‌نشینی (میلی‌متر بر ثانیه)
#define MC_HOME_SLOW_X_MM_S     2.0f
#define MC_HOME_SLOW_Y_MM_S     1.0f
#define MC_HOME_SLOW_Z_MM_S     0.5f
// فاصله عقب‌نشینی بین مرحله سریع و آهسته (میلی‌متر)
#define MC_HOME_BACKOFF_X_MM    5.0f
#define MC_HOME_BACKOFF_Y_MM    5.0f
#define MC_HOME_BACKOFF_Z_MM    3.0f
// نقطه امن X و Y که قبل از هومینگ Z نازل به آن می‌رود (میلی‌متر). X برابر 100 یعنی وسط صفحه داغ 200 میلی‌متری روی مسیر 240 میلی‌متری X.
#define MC_SAFE_HOME_X_MM       100.0f
#define MC_SAFE_HOME_Y_MM       60.0f

// سنسور هومینگ Z: 1 = BLTouch ، 2 = لیمیت‌سوئیچ Z.
// فعلاً فقط BLTouch پیاده‌سازی و روی دستگاه تست شده است. حالت 2 هنوز نوشته نشده
// و روی این برد قابل تست نیست (سیگنال BLTouch روی همان ورودی Z-min است).
#define MC_Z_SENSOR_BLTOUCH     1
#define MC_Z_SENSOR_ENDSTOP     2
#define MC_Z_HOME_SENSOR        MC_Z_SENSOR_BLTOUCH
#if (MC_Z_HOME_SENSOR != MC_Z_SENSOR_BLTOUCH)
#error "MC_Z_HOME_SENSOR: فعلاً فقط BLTouch پشتیبانی می‌شود (مقدار 1)."
#endif

// ===================== ۵. پراب BLTouch و G30 (بند ۶.۵ و ۶.۶) =====================
#define MC_PROBE_MAX_MM             10.0f   // سقف مسافت پایین‌رفتن در G30
#define MC_PROBE_APPROACH_MM_S      1.0f    // سرعت پایین‌رفتن تا برخورد
#define MC_PROBE_RETURN_MM_S        2.0f    // سرعت برگشت به ارتفاع شروع
// آفست پیش‌فرض پراب نسبت به نازل (M851 عوضش می‌کند)
#define MC_PROBE_OFFSET_X_MM        0.0f
#define MC_PROBE_OFFSET_Y_MM        0.0f
#define MC_PROBE_OFFSET_Z_MM        0.0f
// حد مجاز قدر مطلق آفست
#define MC_PROBE_OFFSET_XY_LIMIT_MM 50.0f
#define MC_PROBE_OFFSET_Z_LIMIT_MM  20.0f
// زمان‌بندی BLTouch (میلی‌ثانیه)
#define MC_BLTOUCH_HOLD_MS          500U    // مدت پالس هر فرمان
#define MC_BLTOUCH_SETTLE_MS        300U    // صبر بعد از Deploy و Stow
#define MC_BLTOUCH_SELFTEST_MS      3000U   // صبر بعد از SelfTest
#define MC_BLTOUCH_ALARM_MS         500U    // صبر بعد از AlarmRelease

// ===================== ۶. لیمیت‌سوئیچ‌ها (بند ۶.۱) =====================
#define MC_ENDSTOP_DEBOUNCE_MS      5U      // حداقل مدت پایداری سیگنال

// ===================== ۷. درایور موتورها =====================
// خاموشی خودکار درایورها بعد از این مدت بیکاری (میلی‌ثانیه). بعداً با M84 و تنظیمات کامل می‌شود.
#define MC_STEPPER_IDLE_DISABLE_MS  30000U

// ===================== ۸. دما (بند ۴) =====================
// همه دماها به درجه سانتی‌گراد.
// حداقل و حداکثر دما (خارج از این بازه = خطا و خاموشی). سقف هدف = حداکثر منهای فاصله امن.
#define MC_HOTEND_MIN_TEMP_C        5
#define MC_HOTEND_MAX_TEMP_C        275
#define MC_HOTEND_TARGET_BUFFER_C   15
#define MC_BED_MIN_TEMP_C           5
#define MC_BED_MAX_TEMP_C           130
#define MC_BED_TARGET_BUFFER_C      10
// حداقل دمای اکستروژن (M302)
#define MC_EXTRUDE_MIN_TEMP_C       170
// PID هات‌اند (M303 با U1 عوضش می‌کند)
#define MC_HOTEND_PID_KP            20.0f
#define MC_HOTEND_PID_KI            1.0f
#define MC_HOTEND_PID_KD            100.0f
// حفاظت‌ها: watch heating (زمان دوره بر حسب ثانیه، افزایش لازم و هیسترزیس بر حسب درجه)
#define MC_HOTEND_WATCH_PERIOD_S    20U
#define MC_HOTEND_WATCH_RISE_C      2
#define MC_HOTEND_RUNAWAY_HYST_C    4
#define MC_HOTEND_RUNAWAY_PERIOD_S  40U
#define MC_HOTEND_FAILED_PERIOD_S   600U
#define MC_BED_WATCH_PERIOD_S       60U
#define MC_BED_WATCH_RISE_C         2
#define MC_BED_RUNAWAY_HYST_C       4
#define MC_BED_RUNAWAY_PERIOD_S     60U
#define MC_BED_FAILED_PERIOD_S      1800U
// انتظار برای دما (M109 و M190): پنجره ±درجه، و مدت ماندن داخل پنجره (میلی‌ثانیه)
#define MC_TEMP_WAIT_WINDOW_C       1
#define MC_TEMP_WAIT_RESIDENCY_MS   10000U
// M303: تعداد چرخه و زمان کل پیش‌فرض
#define MC_AUTOTUNE_CYCLES_DEFAULT  5
#define MC_AUTOTUNE_TIMEOUT_S       1200U

// ===================== ۹. ارتباط با کامپیوتر (بند ۳) =====================
// فاصله پیام echo:busy: processing در دستور طولانی (میلی‌ثانیه)
#define MC_BUSY_REPLY_INTERVAL_MS   2000U

// ===================== ۱۰. ذخیره‌سازی EEPROM (بند ۷.۱) =====================
#define MC_EEPROM_I2C_ADDR          0x50    // آدرس I2C هفت‌بیتی تراشه AT24C32D (A0 تا A2 به GND)
#define MC_EEPROM_SIZE_BYTES        4096    // ظرفیت تراشه به بایت (32 کیلوبیت)
#define MC_EEPROM_PAGE_BYTES        32      // اندازه صفحه نوشتن به بایت (یک نوشتن نباید از مرز صفحه رد شود)
#define MC_EEPROM_I2C_TIMEOUT_MS    20      // حداکثر زمان یک انتقال I2C (میلی‌ثانیه)
#define MC_EEPROM_WRITE_TIMEOUT_MS  25      // حداکثر انتظار تراشه بعد از هر نوشتن (میلی‌ثانیه؛ زمان واقعی حدود 5)
#define MC_EEPROM_POLL_PERIOD_MS    2       // دوره وظیفه درایور (میلی‌ثانیه)
#define MC_EEPROM_TEST_ADDR         4032    // شروع «ناحیه تست» (آخرین 64 بایت حافظه؛ فقط فرمان‌های آزمایشی اینجا می‌نویسند)
#define MC_EEPROM_TEST_LEN          64      // اندازه ناحیه تست به بایت
#define MC_EEPROM_TEST_MIS_OFFSET   20      // فاصله شروع نوشتن «نامرتب» از ابتدای ناحیه تست (برای رد شدن از مرز صفحه)
#define MC_EEPROM_TEST_MIS_LEN      40      // طول نوشتن «نامرتب» (بایت)
