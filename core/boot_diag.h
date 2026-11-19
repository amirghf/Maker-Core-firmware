#pragma once
// تشخیص بند ۶.۴: کپی مقدار و آدرس چند متغیر debug برای پیدا کردن علت اختلاف Live Expressions.
namespace mc {

// یک بار در انتهای mc_core_init_c صدا زده می‌شود.
void mc_diag_init();

// هر 100 میلی‌ثانیه صدا زده می‌شود.
void mc_diag_poll();

}  // namespace mc
