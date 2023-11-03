#include <kmod.h>
#include <stdio.h>
#include <hal/timer.h>

#define TIMER_DURATION              5000000UL // timer duration to wait for until exiting

/* KERNEL MODULE INITIALIZATION FUNCTION */
int32_t kmod_init() {
    if(timer_tick == 0) {
        printf("System timer seems to not be working (tick is still 0), exiting to avoid hangs");
        return -1;
    }
    timer_tick_t t_start = timer_tick;
    while(timer_tick - t_start <= TIMER_DURATION) {
        timer_tick_t elapsed = timer_tick - t_start;
        timer_tick_t elapsed_sec = elapsed / 1000000UL;
        kprintf("Systick:%20llu, %02lu:%02u:%02u.%06lu elapsed\r", (uint64_t) timer_tick, (uint32_t)(elapsed_sec / 3600), (uint8_t)((elapsed_sec / 600) % 60), (uint8_t)(elapsed_sec % 60), (uint32_t)(elapsed - elapsed_sec * 1000000UL));
    }
    kprintf("\nSystem timer is OK\n");
    return 0;
}