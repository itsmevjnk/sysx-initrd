include $(WORKDIR)/target.mk

.PHONY: all clean modules binaries

all: initrd.tar

modules:
	make -C modules

binaries:
	make -C bin

initrd.tar: kernel.sym modules binaries
	rm -rf root
	mkdir -p root/bin
	mkdir -p root/boot/modules
	mkdir -p root/dev
	mkdir -p root/tmp
	touch root/boot/initrd.tar
	cp kernel.sym root/boot/kernel.sym
	make install -C modules
	make install -C bin
	cd root && tar -cvf ../$@ * && cd ..

kernel.sym: $(WORKDIR)/kernel/kernel.elf
	$(OC) --extract-symbol $< $@

clean:
	rm kernel.sym
	rm -rf root/
	make clean -C modules
	make clean -C bin
