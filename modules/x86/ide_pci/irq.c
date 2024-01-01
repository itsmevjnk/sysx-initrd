#include "irq.h"
#include "regs.h"
#include <drivers/pci.h>

ide_channel_devtree_t* ide_first_channel = NULL;

void ide_pci_irq_handler(size_t irq, void* context) {
    (void) context;

    // kdebug("PCI native mode interrupt %u", irq);

    size_t channels = 0; // number of channels we affected
    ide_channel_devtree_t* channel = ide_first_channel; // iterate through each channel that we've got and figure out which interrupt this one came from
    while(channel != NULL) {
        if(channel->irq_line == irq && mutex_test(&channel->irq_block)) {
            ide_read_byte(channel, IDE_REG_STAT); // so that the channel de-asserts its interrupt
            channel->irq_block.locked = 0; // signal to calling function that there's an interrupt
            channels++;
        }
        channel = channel->next;
    }

    if(!channels) {
        /* we haven't figured out which channel this interrupt came from, so we should assume it's coming from any channel */
        channel = ide_first_channel; // iterate through each channel that we've got and figure out which interrupt this one came from
        while(channel != NULL) {
            if(channel->irq_line == irq) {
                ide_read_byte(channel, IDE_REG_STAT);
                channels++;
            }
            channel = channel->next;
        }

        if(!channels) kdebug("bogus IRQ %u", irq);
    }
}

void ide_compat_irq_handler(size_t irq, void* context) {
    (void) context;

    // kdebug("ISA compatibility mode interrupt %u", irq);
    
    size_t channels = 0;
    ide_channel_devtree_t* channel = ide_first_channel; // iterate through each channel that we've got and figure out which interrupt this one came from
    while(channel != NULL) {
        if(channel->irq_line == irq) {
            if(mutex_test(&channel->irq_block)) mutex_release(&channel->irq_block); // signal to calling function that there's an interrupt
            ide_read_byte(channel, IDE_REG_STAT); // de-assert interrupt
            channels++;
        }
        channel = channel->next;
    }

    if(!channels) kdebug("bogus IRQ %u", irq);    
}