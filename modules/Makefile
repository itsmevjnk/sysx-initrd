include $(WORKDIR)/target.mk
include list/$(TARGET).mk

.PHONY: all install clean

all:
	for mod in $(MODS); do \
		$(MAKE) -C $$mod ; \
	done

install:
	for mod in $(MODS); do \
		$(MAKE) install -C $$mod ; \
	done

clean:
	for mod in $(MODS); do \
		$(MAKE) clean -C $$mod ; \
	done