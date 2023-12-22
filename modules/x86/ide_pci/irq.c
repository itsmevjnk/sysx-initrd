#include "irq.h"
#include "regs.h"
#include <drivers/pci.h>

ide_channel_devtree_t* ide_irq_channels[24] = {NULL};

// void ide_pci_irq_handler(uint8_t irq, void* context) {
//     (void) context;

//     // kdebug("PCI native mode interrupt %u", irq);
//     if(ide_irq_channels[irq] == NULL) {
//         kdebug("bogus IRQ %u", irq);
//         return;
//     }

//     size_t channels = 0; // number of channels we affected
//     ide_channel_devtree_t* channel = (ide_channel_devtree_t*) ((pci_devtree_t*) ide_irq_channels[irq]->header.parent)->header.first_child; // iterate through each channel in the PCI device that sent this interrupt
//     while(channel != NULL) {
//         if(mutex_test(&channel->irq_block)) {
//             ide_read_byte(channel, IDE_REG_STAT); // so that the channel de-asserts its interrupt
//             channel->irq_block.locked = 0; // signal to calling function that there's an interrupt
//             channels++;
//         }
//         channel = (ide_channel_devtree_t*) channel->header.next_sibling;
//     }

//     if(!channels) {
//         /* we haven't figured out which channel this interrupt came from, so we should assume it's coming from any channel */
//         channel = (ide_channel_devtree_t*) ((pci_devtree_t*) ide_irq_channels[irq]->header.parent)->header.first_child;
//         while(channel != NULL) {
//             ide_read_byte(channel, IDE_REG_STAT); // so that the channel de-asserts its interrupt
//             channel = (ide_channel_devtree_t*) channel->header.next_sibling;
//         }
//     }
// }

void ide_compat_irq_handler(uint8_t irq, void* context) {
    (void) context;

    // kdebug("ISA compatibility mode interrupt %u", irq);
    if(ide_irq_channels[irq] == NULL) {
        kdebug("bogus IRQ %u", irq);
        return;
    }

    if(mutex_test(&ide_irq_channels[irq]->irq_block)) mutex_release(&ide_irq_channels[irq]->irq_block); // signal to calling function that there's an interrupt
    ide_read_byte(ide_irq_channels[irq], IDE_REG_STAT); // de-assert interrupt
}