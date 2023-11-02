include $(WORKDIR)/target.mk

OUTPUT_FILE=hello.ko # output file name

# component objects
OBJS=\
main.o \
hello.o

.PHONY: all clean install
.SUFFIXES: .o .c .s .asm

all: $(OUTPUT_FILE)

$(OUTPUT_FILE): $(OBJS)
	$(LD) -r $(OBJS) -o $@

.c.o:
	$(CC) -c $< -o $@ $(CFLAGS) -isystem $(WORKDIR)/initrd/modules/include -isystem $(WORKDIR)/kernel -isystem $(WORKDIR)/kernel/lib

.s.o:
	$(AS) -c $< -o $@ $(ASFLAGS)

.asm.o:
	$(ASNG) $< -o $@ $(ASNGFLAGS)

install: $(OUTPUT_FILE)
	mkdir -p $(WORKDIR)/initrd/root/boot/modules
	cp $(OUTPUT_FILE) $(WORKDIR)/initrd/root/boot/modules

clean:
	rm -f $(OUTPUT_FILE)
	rm -f $(OBJS)