include $(WORKDIR)/target.mk

.PHONY: all clean modules

all: initrd.tar

modules:
	make -C modules

initrd.tar: kernel.sym modules
	rm -rf root
	mkdir -p root/boot/modules
	mkdir -p root/dev
	mkdir -p root/tmp
	touch root/boot/initrd.tar
	cp kernel.sym root/boot/kernel.sym
	make install -C modules
	cd root && tar -cvf ../$@ * && cd ..

kernel.sym: $(WORKDIR)/kernel/kernel.elf
	$(OC) --extract-symbol $< $@

clean:
	rm kernel.sym
	rm -rf root/
	make clean -C modules
