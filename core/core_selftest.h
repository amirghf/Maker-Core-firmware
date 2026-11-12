#pragma once
// سلف‌تست‌های هسته. این هدر از main.c (زبان C) هم include می‌شود.

#ifdef __cplusplus
extern "C" {
#endif

// تست بافر حلقوی. 1 = سالم. نتیجه را در mc_ring_selftest_ok هم می‌گذارد.
int mc_ring_selftest_c(void);

// تست زمان‌بند (حدود ۱۰۰ میلی‌ثانیه طول می‌کشد). 1 = سالم. نتیجه را در mc_sched_selftest_ok هم می‌گذارد.
int mc_sched_selftest_c(void);

#ifdef __cplusplus
}
#endif
