#pragma once
// رنگ‌های عمومی و مشترک رابط کاربری (بند ۸.۷).
// هیچ هدر ST یا HAL اینجا include نمی‌شود.
#include "../core/gfx.h"

namespace mc {

constexpr uint16_t kScreenBg     = mc_rgb565(8, 10, 16);       // پس‌زمینه اصلی صفحه
constexpr uint16_t kTopBarBg     = mc_rgb565(16, 44, 104);     // نوار بالا
constexpr uint16_t kAccentLine   = mc_rgb565(0, 200, 230);     // خط زیر نوار بالا
constexpr uint16_t kCardBg       = mc_rgb565(22, 27, 38);      // پس‌زمینه کارت‌ها و دکمه‌ها
constexpr uint16_t kCardBorder   = mc_rgb565(48, 58, 80);      // حاشیه کارت‌ها
constexpr uint16_t kBarTrack     = mc_rgb565(44, 52, 70);      // شیار نوار پیشرفت
constexpr uint16_t kBtnBg        = mc_rgb565(10, 30, 70);      // پس‌زمینه دکمه منو
constexpr uint16_t kBtnBorder    = mc_rgb565(60, 110, 200);    // حاشیه دکمه منو
constexpr uint16_t kAccNozzle    = mc_rgb565(255, 120, 30);    // رنگ شاخص اکسترودر
constexpr uint16_t kAccFan       = mc_rgb565(0, 200, 230);     // رنگ شاخص فن
constexpr uint16_t kAccBed       = mc_rgb565(240, 70, 70);     // رنگ شاخص بستر
constexpr uint16_t kIconBody     = mc_rgb565(120, 135, 165);   // بدنه آیکون‌ها
constexpr uint16_t kFanBody      = mc_rgb565(46, 58, 82);      // بدنه قاب آیکون فن
constexpr uint16_t kFanDisk      = mc_rgb565(10, 18, 28);      // دیسک داخلی آیکون فن
constexpr uint16_t kYellowBusy   = mc_rgb565(255, 200, 0);     // رنگ زرد اشتغال / USB
constexpr uint16_t kLabelGray    = mc_rgb565(180, 180, 180);   // برچسب‌ها و متون فرعی
constexpr uint16_t kDividerGray  = mc_rgb565(60, 70, 90);      // جداکننده‌ها

}  // namespace mc
