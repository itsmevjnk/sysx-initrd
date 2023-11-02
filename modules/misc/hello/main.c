#include <kmod.h>

void hello(); // in hello.c

/* KERNEL MODULE INITIALIZATION FUNCTION */
__attribute__((unused)) // this will be used by the kernel
static int32_t kmod_init() {
    hello();
    return 0;    
}