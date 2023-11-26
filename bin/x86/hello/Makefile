include $(WORKDIR)/target.mk

OUTPUT_FILE=hello # output file name

# component objects
OBJS=\
main.o

.PHONY: all clean install
.SUFFIXES: .o .c .s .asm

all: $(OUTPUT_FILE)

$(OUTPUT_FILE): $(OBJS)
	$(LD) $(OBJS) -o $@

.c.o:
	$(CC) -c $< -o $@ $(CFLAGS)

.s.o:
	$(AS) -c $< -o $@ $(ASFLAGS)

.asm.o:
	$(ASNG) $< -o $@ $(ASNGFLAGS)

install: $(OUTPUT_FILE)
	mkdir -p $(WORKDIR)/initrd/root/bin
	cp $(OUTPUT_FILE) $(WORKDIR)/initrd/root/bin

clean:
	rm -f $(OUTPUT_FILE)
	rm -f $(OBJS)