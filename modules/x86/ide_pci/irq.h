#ifndef IDE_IRQ_H
#define IDE_IRQ_H

#include <kmod.h>
#include "devtree_defs.h"

extern ide_channel_devtree_t* ide_irq_channels[16]; // IRQ line to channel mapping

void ide_pci_irq_handler(uint8_t irq, void* context); // PCI native mode IRQ handler (one IRQ for each/both channels)
void ide_compat_irq_handler(uint8_t irq, void* context); // ISA compatibility mode IRQ handler (one IRQ for each channel)

#endif