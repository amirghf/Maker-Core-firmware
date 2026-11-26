#pragma once
// تنظیمات دستگاه MakerCore (مثل Configuration.h در مارلین).
// همه مقدارهای قابل تنظیم دستگاه اینجا هستند. بعد از هر تغییر باید دوباره بیلد کرد.
// این فایل فقط عدد و انتخاب دارد و هیچ include ای ندارد.
// (انتخاب برد در بند ۱۱.۲ به همین فایل اضافه می‌شود.)
// نسخه و نام فرمویر در mc_version.h است.

// ===================== ۱. مکانیک (بند ۵.۸) =====================
// گام بر میلی‌متر هر محور. بند 11.1: با تست روی دستگاه و جدول دقیق کالیبره شد.
// X برابر 42.11، Y برابر 386.53، Z برابر 421.05 (تأیید نهایی با چاپ مکعب).
// E برابر 100 است و هنوز کالیبره نشده (کالیبراسیون E بعداً انجام می‌شود).
// با دستور M92 می‌شود موقتی عوض کرد (تا ریست)، و با M500 در EEPROM ذخیره می‌شود.
#define MC_STEPS_PER_MM_X   42.11f
#define MC_STEPS_PER_MM_Y   386.53f
#define MC_STEPS_PER_MM_Z   421.05f
#define MC_STEPS_PER_MM_E   100.0f

#define MC_X_DIR_INVERT   0   // وارونه‌سازی جهت موتور X (0 = عادی، 1 = برعکس)
#define MC_Y_DIR_INVERT   1   // وارونه‌سازی جهت موتور Y (0 = عادی، 1 = برعکس). بند 6.3: سوئیچ Y جلوی دستگاه است
#define MC_Z_DIR_INVERT   0   // وارونه‌سازی جهت موتور Z (0 = عادی، 1 = برعکس)
#define MC_E_DIR_INVERT   1   // وارونه‌سازی جهت موتور اکسترودر E (0 = عادی، 1 = برعکس). بند 11.3: جهت قبلی برعکس بود

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
#define MC_MOTION_ARM_AT_BOOT          1     // مقدار اولیه mc_dbg_80_mv_arm بعد از بوت: 1 = حرکت مجاز (دکمه‌های نمایشگر و G-code کار می‌کنند)، 0 = حرکت تا نوشتن 1 در Live Expressions ممنوع

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
// بالا بردن Z در شروع G28 قبل از حرکت دیگر محورها (میلی‌متر و سرعت به میلی‌متر بر ثانیه) (بند ۶.۳ و ۶.۷)
// وقتی Z هوم‌نشده است نسبی و فقط یک‌بار؛ وقتی Z هوم‌شده است تا همین ارتفاع مطلق
#define MC_HOME_Z_LIFT_MM                 20.0f
#define MC_HOME_Z_LIFT_MM_S               2.0f
#define MC_HOME_Z_LIFT_FORGET_ON_DISABLE  0     // 0 = وقتی درایور Z خاموش شد، «بالا رفته» را فراموش نکن. 1 = فراموش کن (برای پرینتری که Z بدون برق پایین می‌افتد) (بند ۶.۳ و ۶.۷)


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
// حدود ناحیه امن پراب روی بستر (میلی‌متر، بند ۱۰.۹؛ کاربر باید مقدار واقعی را بعداً با خط‌کش اندازه‌گیری و تنظیم کند)
#define MC_PROBE_SAFE_MIN_X_MM      0.0f
#define MC_PROBE_SAFE_MAX_X_MM      240.0f
#define MC_PROBE_SAFE_MIN_Y_MM      0.0f
#define MC_PROBE_SAFE_MAX_Y_MM      120.0f
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

// ===================== ۱۱. ذخیره‌سازی سخت‌دار تنظیمات (بند ۷.۲) =====================
#define MC_SETTINGS_SLOT_A_ADDR          0       // آدرس شروع اسلات A در EEPROM (بایت)
#define MC_SETTINGS_SLOT_B_ADDR          256     // آدرس شروع اسلات B در EEPROM (بایت؛ باید بعد از پایان اسلات A باشد)
#define MC_SETTINGS_SLOT_BYTES           256     // اندازه هر اسلات به بایت (سرآیند + داده؛ ناحیه تست 4032 تا 4095 نباید با اسلات‌ها هم‌پوشانی داشته باشد)
#define MC_SETTINGS_HEADER_BYTES         16      // اندازه سرآیند هر اسلات به بایت (ثابت فرمت؛ عوض نشود)
#define MC_SETTINGS_MAGIC                0x4D43534EUL  // عدد شناسایی اسلات (حروف MCSN)
#define MC_SETTINGS_FORMAT_VERSION       1       // نسخه فرمت ذخیره‌سازی (اگر ساختار عوض شد زیاد شود)
#define MC_SETTINGS_TEST_PAYLOAD_LEN     48      // طول داده آزمایشی برای فرمان‌های تست (بایت)
#define MC_SETTINGS_POLL_PERIOD_MS       5       // دوره وظیفه settings_store (میلی‌ثانیه)
#define MC_SETTINGS_STEP_TIMEOUT_MS      500     // حداکثر زمان هر مرحله (خواندن یا نوشتن یک اسلات) به میلی‌ثانیه

// ===================== ۱۲. تنظیمات و ذخیره‌سازی M500 تا M503 (بند ۷.۳) =====================
#define MC_CFG_PAYLOAD_VERSION           1       // نسخه قالب داده تنظیمات (اگر چیدمان مقدارها عوض شد زیاد شود)
#define MC_CFG_CMD_TIMEOUT_MS            3000    // حداکثر زمان انتظار M500 و M501 برای تمام‌شدن کار، به میلی‌ثانیه
#define MC_CFG_BOOT_TIMEOUT_MS           3000    // حداکثر زمان خواندن خودکار تنظیمات هنگام بوت (میلی‌ثانیه)؛ بعد از آن پیش‌فرض‌ها می‌مانند (بند ۷.۴)
#define MC_CFG_BOOT_POLL_MS              10      // دوره وظیفه خواندن خودکار تنظیمات هنگام بوت (میلی‌ثانیه) (بند ۷.۴)

// ===================== ۱۳. صفحه اصلی (بند ۸.۶ و ۸.۷) =====================
#define MC_UI_POLL_MS            20      // دوره وظیفه صفحه اصلی (میلی‌ثانیه). در هر دور فقط یک مرحله یا یک فیلد نقاشی می‌شود
#define MC_UI_SPLASH_HOLD_MS     2000    // مدت نمایش اسپلش قبل از رفتن به صفحه اصلی (میلی‌ثانیه)
#define MC_DEVICE_NAME           "MakerCore"   // نام دستگاه که در نوار بالای صفحه اصلی نشان داده می‌شود. کاربر آن را عوض می‌کند. حداکثر ۲۴ کاراکتر انگلیسی (بیشتر بریده می‌شود)
#define MC_UI_TEMP_NEAR_X10      30      // اختلاف مجاز دمای فعلی با هدف برای نمایش READY، برحسب دهم درجه (30 = 3.0 درجه). بیشتر از آن HEAT یا COOL
#define MC_UI_HOME_MSG_MS        3000    // مدت نمایش پیام علت رد شدن دکمه‌های صفحه اصلی در نوار بالا (میلی‌ثانیه)
#define MC_UI_HOME_MOVE_HOLD_MS  150     // بعد از آخرین پالس STEP یک محور، تا این مدت «در حال حرکت» حساب می‌شود (میلی‌ثانیه)
#define MC_UI_HOME_ARROW_FLIP_MASK 0     // برعکس‌کردن شکل فلش‌ها برای هر محور: بیت 0 = X، 1 = Y، 2 = Z، 3 = E (1 = جهت مثبت فلش چپ یا پایین)
#define MC_UI_SCREEN_SLEEP_MS    60000   // مقدار پیش‌فرض مدت بیکاری تا خاموشی نمایشگر (میلی‌ثانیه) تا وقتی از صفحه DISPLAY ذخیره نشده باشد. 0 = هیچ‌وقت خاموش نشود
#define MC_UI_MENU_EXIT_MS         30000   // مقدار پیش‌فرض مدت بیکاری تا برگشت خودکار از منو و صفحه‌های داخلی به صفحه اصلی (میلی‌ثانیه). 0 = هیچ‌وقت
#define MC_UI_DISP_MAX_S           3600    // سقف مجاز هر دو زمان ذخیره‌شده در صفحه DISPLAY (ثانیه)
#define MC_UI_DISP_SAVE_DELAY_MS   1000    // مکث بعد از آخرین تغییر در صفحه DISPLAY تا ذخیره در EEPROM (میلی‌ثانیه)

// ===================== ۱۴. انکودر و بوق (بند ۸.۸) =====================
#define MC_ENC_POLL_MS           1       // دوره خواندن انکودر (میلی‌ثانیه)
#define MC_ENC_STEPS_PER_CLICK   4       // تعداد گام کوادراتور برای هر کلیک مکانیکی
#define MC_ENC_REVERSE           0       // جهت چرخش (0 = عادی، 1 = برعکس)
#define MC_ENC_BTN_DEBOUNCE_MS   20      // مدت ضدلرزش دکمه انکودر (میلی‌ثانیه)
#define MC_ENC_PRESS_BEEP_MS     40      // طول بوق فشردن دکمه (میلی‌ثانیه)

// ===================== ۱۵. صفحه اکسترودر (بند ۸.۹) =====================
#define MC_UI_EX_HEAT_TEMP_C           200   // دمای هدف دکمه HEAT در صفحه اکسترودر (درجه سانتی‌گراد)؛ برای نمایش متن دکمه و فعال‌شدن رنگ نارنجی
#define MC_UI_EX_FEED_STEP_1_MM        1     // گام اول FEED (میلی‌متر)
#define MC_UI_EX_FEED_STEP_2_MM        5     // گام دوم FEED (میلی‌متر)
#define MC_UI_EX_FEED_STEP_3_MM        10    // گام سوم FEED (میلی‌متر)
#define MC_UI_EX_FEED_STEP_DEFAULT_IDX 1     // اندیس چیپ فعال هنگام باز شدن صفحه (۰ تا ۲)
#define MC_UI_EX_FEED_RATE_MM_MIN      300   // سرعت حرکت FEED دکمه‌های صفحه اکسترودر (میلی‌متر بر دقیقه)
#define MC_UI_REFUSE_BEEP_MS           300   // طول بوق بلند «دکمه رد شد» در صفحه‌های رابط کاربری (میلی‌ثانیه)
#define MC_UI_EX_MSG_MS                3000  // مدت نمایش پیام علت رد شدن در صفحه اکسترودر (میلی‌ثانیه)
#define MC_UI_MOVE_FEED_X_MM_MIN       3000  // سرعت حرکت دکمه‌های X صفحه حرکت (میلی‌متر بر دقیقه؛ برنامه‌ریز آن را به سقف سرعت محور محدود می‌کند)
#define MC_UI_MOVE_FEED_Y_MM_MIN       600   // سرعت حرکت دکمه‌های Y صفحه حرکت (میلی‌متر بر دقیقه)
#define MC_UI_MOVE_FEED_Z_MM_MIN       150   // سرعت حرکت دکمه‌های Z صفحه حرکت (میلی‌متر بر دقیقه)
#define MC_UI_MOVE_MSG_MS              3000  // مدت نمایش پیام علت رد شدن در نوار بالای صفحه حرکت (میلی‌ثانیه)
#define MC_UI_SEN_MSG_MS               3000  // مدت نمایش پیام علت رد شدن در نوار بالای صفحه سنسورها (میلی‌ثانیه)

// ===================== ۱۶. صفحه داغ (بند ۸.۱۰) =====================
#define MC_UI_BED_PRESET_1_C           100   // دمای هدف دکمه HEAT اول در صفحه داغ (درجه سانتی‌گراد؛ سقف هدف بستر 120 است)
#define MC_UI_BED_PRESET_2_C           120   // دمای هدف دکمه HEAT دوم در صفحه داغ (درجه سانتی‌گراد؛ سقف هدف بستر 120 است)

// ===================== ۱۷. صفحه سنسورها (بند ۸.۱۲) =====================
#define MC_UI_FIL_EMPTY_LEVEL          0     // سطح پین سنسور فیلامنت (PA4) وقتی فیلامنت تمام شده (1 = HIGH، 0 = LOW). روی برد واقعی تأیید شد: فیلامنت جا = LOW، فیلامنت نیست = HIGH
#define MC_UI_FIL_PULL_MODE            1     // پول داخلی پین سنسور فیلامنت (PA4): 0 = بدون پول، 1 = پول‌آپ، 2 = پول‌داون
#define MC_UI_FIL_DEBOUNCE_MS          100   // مدتی که سطح پین باید ثابت بماند تا تغییر وضعیت فیلامنت پذیرفته شود (میلی‌ثانیه)

// ===================== ۱۸. ورودی لمس و کالیبراسیون (بند ۸.۱۵) =====================
#define MC_TOUCH_POLL_MS                10      // دوره خواندن لمس (میلی‌ثانیه)
#define MC_TOUCH_DOWN_CONFIRM           2       // تعداد دور پشت‌سرهم لمس‌شده تا فشردن تأیید شود
#define MC_TOUCH_UP_CONFIRM           3       // تعداد دور پشت‌سرهم بدون لمس تا رها شدن تأیید شود
#define MC_TOUCH_CAL_ADDR               512     // آدرس رکورد کالیبراسیون لمس در EEPROM (بایت؛ جدا از اسلات‌های تنظیمات و ناحیه تست)
#define MC_TOUCH_CAL_MAGIC              0x4C414354UL  // عدد شناسایی رکورد (حروف TCAL)
#define MC_TOUCH_CAL_FORMAT_VERSION     1       // نسخه فرمت رکورد (اگر ساختار عوض شد زیاد شود)
#define MC_TOUCH_CAL_STORE_POLL_MS      5       // دوره ماشین حالت ذخیره (میلی‌ثانیه)
#define MC_TOUCH_CAL_STORE_TIMEOUT_MS   500     // حداکثر زمان یک کار ذخیره یا بارگذاری (میلی‌ثانیه)
#define MC_DISPSET_ADDR              576     // آدرس رکورد تنظیمات نمایشگر در EEPROM (بایت؛ جدا از اسلات‌های تنظیمات، رکورد لمس 512 و ناحیه تست 4032)
#define MC_DISPSET_MAGIC             0x53505344UL  // عدد شناسایی رکورد (حروف DSPS)
#define MC_DISPSET_FORMAT_VERSION    1       // نسخه فرمت رکورد (اگر ساختار عوض شد زیاد شود)
#define MC_DISPSET_POLL_MS           5       // دوره ماشین حالت ذخیره (میلی‌ثانیه)
#define MC_DISPSET_TIMEOUT_MS        500     // حداکثر زمان یک کار ذخیره یا بارگذاری (میلی‌ثانیه)
#define MC_TOUCH_CAL_MIN_SPREAD         300     // کمترین فاصله خام بین دو نقطه کالیبراسیون (واحد خام لمس 0 تا 4095)
#define MC_TOUCH_CAL_RELEASE_MS         300     // کمترین مکث بعد از رها شدن انگشت بین دو نقطه (میلی‌ثانیه)
#define MC_TOUCH_CAL_MAX_RETRY          3       // حداکثر تکرار خودکار بعد از شکست محاسبه
#define MC_TOUCH_TEST_MS                20000   // مدت صفحه آزمون (میلی‌ثانیه)
#define MC_UI_TOUCH_BEEP_MS             40      // طول بوق ضربه لمس (میلی‌ثانیه)
#define MC_UI_TOUCH_TAP_SLOP_PX         14      // بیشترین جابه‌جایی انگشت (پیکسل) تا حرکت هنوز «ضربه» حساب شود
#define MC_UI_TOUCH_SWIPE_MIN_PX        40      // کمترین جابه‌جایی عمودی (پیکسل) تا حرکت «کشیدن» حساب شود
#define MC_UI_TOUCH_HIT_MARGIN_PX       4       // حاشیه اضافه دور هر دکمه برای راحت‌تر زدن با انگشت (پیکسل)

// ===================== ۱۹. تعویض فیلامنت M600 (بند ۹.۱) =====================
#define MC_FILCHG_RETRACT_MM           2.0f   // جمع‌کردن اولیه فیلامنت قبل از پارک (میلی‌متر) — پارامتر E
#define MC_FILCHG_UNLOAD_MM            40.0f  // طول بیرون‌کشیدن فیلامنت (میلی‌متر) — پارامتر U
#define MC_FILCHG_LOAD_MM              40.0f  // طول وارد کردن فیلامنت بعد از تعویض (میلی‌متر) — پارامتر L
#define MC_FILCHG_PURGE_MM             20.0f  // طول پرج بعد از بارگذاری (میلی‌متر)
#define MC_FILCHG_MAX_LEN_MM           300.0f // بیشترین مقدار مجاز E/U/L/Z در دستور M600 (میلی‌متر)
#define MC_FILCHG_RETRACT_FEED_MM_MIN  300.0f // سرعت جمع‌کردن اولیه و بیرون‌کشیدن (میلی‌متر بر دقیقه)
#define MC_FILCHG_LOAD_FEED_MM_MIN     300.0f // سرعت وارد کردن (میلی‌متر بر دقیقه)
#define MC_FILCHG_PURGE_FEED_MM_MIN    120.0f // سرعت پرج (میلی‌متر بر دقیقه)
#define MC_FILCHG_PARK_X_MM            10.0f  // موقعیت پارک X (میلی‌متر، مطلق)
#define MC_FILCHG_PARK_Y_MM            10.0f  // موقعیت پارک Y (میلی‌متر، مطلق)
#define MC_FILCHG_Z_LIFT_MM            10.0f  // مقدار بالا بردن Z هنگام پارک (میلی‌متر، نسبی؛ از حد نرم‌افزاری Z بالاتر نمی‌رود)
#define MC_FILCHG_PARK_FEED_MM_MIN     1200.0f// سرعت پارک X/Y و برگشت (میلی‌متر بر دقیقه؛ برنامه‌ریز به سقف سرعت هر محور محدودش می‌کند)
#define MC_FILCHG_Z_FEED_MM_MIN        120.0f // سرعت حرکت Z (میلی‌متر بر دقیقه)
#define MC_FILCHG_HEAT_BAND_C          3      // نازل وقتی «گرم» حساب می‌شود که دمایش حداقل (هدف منهای این مقدار) باشد (درجه)
#define MC_FILCHG_HEAT_TIMEOUT_S       300    // بیشترین مدت صبر برای گرم‌شدن نازل؛ بعد از آن M600 با خطا تمام می‌شود (ثانیه)
#define MC_FILCHG_HEATER_HOLD_S        300    // مدت نگه‌داشتن هیتر در حالت انتظار کاربر؛ بعد از آن هیتر خاموش می‌شود و موقع «ادامه» دوباره گرم می‌شود (ثانیه، 0 = هیچ‌وقت خاموش نشود)
#define MC_FILCHG_BEEP_PERIOD_S        5      // فاصله بوق‌های یادآوری در حالت انتظار کاربر (ثانیه، 0 = بدون بوق)
#define MC_FILCHG_BEEP_MS              150    // طول هر بوق (میلی‌ثانیه)
#define MC_FILCHG_KEY_DEBOUNCE_MS      50     // مدت ثابت‌ماندن کلید START برای پذیرفتن فشردن (میلی‌ثانیه)

// ===================== ۲۰. تعویض و بارگذاری فیلامنت M701 و M702 (بند ۹.۲) =====================
#define MC_FILLOAD_LOAD_MM            40.0f  // طول پیش‌فرض بارگذاری M701 (میلی‌متر)
#define MC_FILLOAD_UNLOAD_MM          40.0f  // طول پیش‌فرض تخلیه M702 (میلی‌متر)
#define MC_FILLOAD_PURGE_MM           20.0f  // طول پرج بعد از M701 (میلی‌متر، 0 = بدون پرج)

// ===================== ۲۱. سنسور اتمام فیلامنت و واکنش مکث خودکار (بند ۹.۳ و ۹.۴) =====================
#define MC_FILRUN_ENABLE_DEFAULT      1      // حالت اولیه تشخیص اتمام فیلامنت بعد از روشن شدن (1 فعال، 0 غیرفعال)
#define MC_FILRUN_PRINT_WINDOW_S      10     // تا چند ثانیه بعد از آخرین حرکت چاپی، «در حال چاپ» حساب شود (ثانیه)
#define MC_FILRUN_ALERT_BEEP_MS       500    // طول بوق هشدار اتمام فیلامنت (میلی‌ثانیه)

// ===================== ۲۲. صفحه ظاهر پیام‌ها (بند ۱۰.۱) =====================
#define MC_UI_MSG_TEXT_MAX      120   // بیشترین تعداد کاراکتر متن اصلی پیام (بیشتر از آن بریده می‌شود)
#define MC_UI_MSG_DETAIL_MAX    56    // بیشترین تعداد کاراکتر خط جزئیات پیام
#define MC_UI_MSG_LINE_CHARS    28    // بیشترین تعداد کاراکتر هر خط متن اصلی (scale 2، عرض 344 پیکسل)
#define MC_UI_MSG_MAX_LINES     3     // بیشترین تعداد خط متن اصلی

// ===================== ۲۳. بررسی اندستاپ حین چاپ (بند ۱۰.۸) =====================
#define MC_ENDSTOP_PRINT_CHECK_DEFAULT 1 // بررسی اندستاپ‌ها حین چاپ عادی (1 = فعال، 0 = غیرفعال)

// ===================== ۲۴. هشدارها و پیام‌های سیستم (بند ۱۰.۱۳) =====================
#define MC_ALERT_BEEP_MS        300   // طول بوق هشدار عادی سیستم (میلی‌ثانیه)
#define MC_ALERT_REPEAT_MS      5000U // تکرار همان هشدار (همان شناسه) در این مدت نادیده گرفته می‌شود (میلی‌ثانیه) (بند ۱۰.۱۳)
#define MC_UI_MSG_QUEUE_LEN     4U    // تعداد پیام‌های در انتظار نمایش (بند ۱۰.۱۳)

// ===================== ۲۵. ریست خودکار و ایمنی kill (بند ۱۰.۱۴) =====================
#define MC_KILL_OK_GUARD_MS        1500U  // تا این مدت بعد از نمایش صفحه kill هیچ OK قبول نمی‌شود (میلی‌ثانیه)
#define MC_KILL_RESTART_DELAY_MS   600U   // فاصله بین قبول شدن OK و ریست (میلی‌ثانیه)

// ===================== ۲۶. کالیبراسیون و ضریب‌های حرکت (بند ۱۱.۱ و ۱۱.۳) =====================
#define MC_M301_GAIN_MAX               1000.0f  // حداکثر مقدار هر ضریب PID برای M301 (بند ۱۱.۱)
#define MC_SPEED_FACTOR_MIN_PCT        10U      // حداقل ضریب سرعت M220 (درصد) (بند ۱۱.۳)
#define MC_SPEED_FACTOR_MAX_PCT        500U     // حداکثر ضریب سرعت M220 (درصد) (بند ۱۱.۳)
#define MC_SPEED_FACTOR_DEFAULT_PCT    100U     // ضریب سرعت پیش‌فرض M220 (درصد) (بند ۱۱.۳)
#define MC_FLOW_FACTOR_MIN_PCT         10U      // حداقل ضریب جریان M221 (درصد) (بند ۱۱.۳)
#define MC_FLOW_FACTOR_MAX_PCT         200U     // حداکثر ضریب جریان M221 (درصد) (بند ۱۱.۳)
#define MC_FLOW_FACTOR_DEFAULT_PCT     100U     // ضریب جریان پیش‌فرض M221 (درصد) (بند ۱۱.۳)
#define MC_M117_TEXT_MAX               40U      // حداکثر طول متن M117 بر حسب کاراکتر (بند ۱۱.۳)
#define MC_M300_DEFAULT_MS             200U     // طول پیش‌فرض بوق M300 (میلی‌ثانیه) (بند ۱۱.۳)
#define MC_M300_MAX_MS                 1000U    // حداکثر طول بوق M300 (میلی‌ثانیه) (بند ۱۱.۳)





