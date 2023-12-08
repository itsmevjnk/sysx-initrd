include $(WORKDIR)/initrd/modules/list/common.mk

MODS=\
$(COMMON_MODS) \
x86/i8042 \
x86/vbe_bochs \
x86/vbe_generic