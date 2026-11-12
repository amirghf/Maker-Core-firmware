#pragma once
// حلقه اصلی هسته MakerCore. این هدر از main.c (زبان C) هم include می‌شود.

#ifdef __cplusplus
extern "C" {
#endif

// وظیفه‌های هسته را ثبت می‌کند. یک‌بار بعد از راه‌اندازی زمان‌پایه صدا زده شود.
void mc_core_init_c(void);
// هر دور حلقه اصلی main.c صدا زده شود.
void mc_core_loop_c(void);

#ifdef __cplusplus
}
#endif
