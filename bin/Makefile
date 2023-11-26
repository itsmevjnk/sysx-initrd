include $(WORKDIR)/target.mk
include $(TARGET)/binaries.mk

.PHONY: all install clean

all:
	for bin in $(BINS); do \
		$(MAKE) -C $$bin ; \
	done

install:
	for bin in $(BINS); do \
		$(MAKE) install -C $$bin ; \
	done

clean:
	for bin in $(BINS); do \
		$(MAKE) clean -C $$bin ; \
	done