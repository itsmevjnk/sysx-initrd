#include <kmod.h>

void hello(); // in hello.c

/* KERNEL MODULE INITIALIZATION FUNCTION */
int32_t kmod_init() {
    hello();
    return 0;    
}