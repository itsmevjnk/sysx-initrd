#include <kmod.h>

void hello(); // in hello.c

/* KERNEL MODULE INITIALIZATION FUNCTION */
int32_t kmod_init(elf_prgload_t* load_result, size_t load_result_len) {
    (void) load_result; (void) load_result_len;
    hello();
    return -1; // negative return value will result in the kernel module being unloaded upon exiting
}