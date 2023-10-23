include $(WORKDIR)/target.mk

.PHONY: all clean

all: initrd.tar

initrd.tar: kernel.sym
	rm -rf root
	mkdir -p root/boot/modules
	mkdir -p root/dev
	mkdir -p root/tmp
	touch root/boot/initrd.tar
	cp kernel.sym root/boot/kernel.sym
	cd root && tar -cvf ../$@ * && cd ..

kernel.sym: $(WORKDIR)/kernel/kernel.elf
	$(OC) --extract-symbol $< $@

clean:
	rm kernel.sym
	rm -rf root/
