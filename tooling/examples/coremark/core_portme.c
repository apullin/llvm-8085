/*
 * CoreMark port implementation for Intel 8085 (LLVM i8085 backend)
 *
 * All timing functions are stubs — we use the simulator's cycle counter
 * for performance measurement, not CoreMark's internal timing.
 */
#include "coremark.h"

/* Volatile seeds for SEED_VOLATILE method */
#if VALIDATION_RUN
volatile ee_s32 seed1_volatile = 0x3415;
volatile ee_s32 seed2_volatile = 0x3415;
volatile ee_s32 seed3_volatile = 0x66;
#endif
#if PERFORMANCE_RUN
volatile ee_s32 seed1_volatile = 0x0;
volatile ee_s32 seed2_volatile = 0x0;
volatile ee_s32 seed3_volatile = 0x66;
#endif
#if PROFILE_RUN
volatile ee_s32 seed1_volatile = 0x8;
volatile ee_s32 seed2_volatile = 0x8;
volatile ee_s32 seed3_volatile = 0x8;
#endif
volatile ee_s32 seed4_volatile = ITERATIONS;
volatile ee_s32 seed5_volatile = 0;

/* Timing stubs — simulator provides cycle count externally */
static CORETIMETYPE start_time_val, stop_time_val;

void start_time(void) {
    start_time_val = 0;
}

void stop_time(void) {
    stop_time_val = 0;
}

CORE_TICKS get_time(void) {
    return (CORE_TICKS)(stop_time_val - start_time_val);
}

secs_ret time_in_secs(CORE_TICKS ticks) {
    /* Return 100 so CoreMark doesn't complain about "must execute 10 secs" */
    (void)ticks;
    return (secs_ret)100;
}

/* Single execution context */
ee_u32 default_num_contexts = 1;

/* portable_init: no-op for bare metal */
void portable_init(core_portable *p, int *argc, char *argv[]) {
    (void)argc;
    (void)argv;
    p->portable_id = 1;
}

/* portable_fini: no-op for bare metal */
void portable_fini(core_portable *p) {
    p->portable_id = 0;
}

/* ee_printf: stub — no UART on simulator, just return 0 */
int ee_printf(const char *fmt, ...) {
    (void)fmt;
    return 0;
}
