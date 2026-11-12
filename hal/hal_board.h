#pragma once
// رابط مستقل از برد. فقط تعریف توابع؛ هیچ هدر ST اینجا include نمی‌شود.

#ifdef __cplusplus
namespace mc {
// همه خروجی‌های حساس را در حالت امن (خاموش) قرار می‌دهد.
void mc_board_safe_state();
// بررسی می‌کند همه خروجی‌ها واقعاً در حالت امن هستند. true = سالم.
bool mc_board_safe_state_check();
// نام برد (مثلاً "MW-X1") برای پیام‌هایی مثل M115. رشته ثابت است و نباید آزاد شود.
const char* mc_board_name();
}  // namespace mc

extern "C" {
#endif

// پل برای کد C (مثل main.c که CubeMX می‌سازد)
void mc_board_safe_state_c(void);
int  mc_board_safe_state_check_c(void);

#ifdef __cplusplus
}
#endif
