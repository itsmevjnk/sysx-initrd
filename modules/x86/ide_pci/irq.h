#ifndef IDE_IRQ_H
#define IDE_IRQ_H

#include <kmod.h>
#include "devtree_defs.h"

extern ide_channel_devtree_t* ide_first_channel;

void ide_pci_irq_handler(size_t irq, void* context); // PCI native mode IRQ handler (one IRQ for each/both channels)
void ide_compat_irq_handler(size_t irq, void* context); // ISA compatibility mode IRQ handler (one IRQ for each channel)

#endif
