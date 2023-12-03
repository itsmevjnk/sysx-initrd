#include <kmod.h>

void hello(); // in hello.c

/* KERNEL MODULE INITIALIZATION FUNCTION */
int32_t kmod_init() {
    hello();
    return -1; // negative return value will result in the kernel module being unloaded upon exiting
}