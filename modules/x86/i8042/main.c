#include <kmod.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <kernel/log.h>
#include <hal/keyboard.h>
#include <arch/x86/i8259.h>
#include <hal/timer.h>
#include <kernel/cmdline.h>

#include "ps2_io.h"

#include "kbd/scset1.h"
#include "kbd/scset2.h"

/* IRQ line numbers for PS/2 ports */
#define PS2_P1_IRQNUM           1
#define PS2_P2_IRQNUM           12

#define PS2_DATA_TIMEOUT        100000UL // response timeout in microseconds
#define PS2_DATA_BUFLEN         8 // data ring buffer length
#define PS2_DATA_RETRIES        3 // maximum number of retries for sending data

struct ps2_port_status {
    bool ok;
    uint16_t id;
    bool parse; // set to parse data according to ID
    uint8_t data[PS2_DATA_BUFLEN];
    uint8_t data_rdidx, data_wridx;

    /* keyboard handling */
    struct {
        bool enabled; // set if the device is a keyboard
        size_t id; // keyboard ID
        uint8_t scset; // scancode set
        bool brk; // set if break code is being sent
        uint8_t ext; // extension byte (E0/E1) or 0 if there's none
        uint8_t code; // scancode (excl. extension byte)
    } kbd;
};
static struct ps2_port_status ps2_ports[2];

/* PS/2 KEYBOARD INCOMING DATA HANDLER */

static void ps2_kbd_handler(uint8_t port, uint8_t data) {
    switch(ps2_ports[port].kbd.scset) {
        case 1: // scancode set 1
            if(data == 0xE0 || data == 0xE1) ps2_ports[port].kbd.ext = data;
            else {
                ps2_ports[port].kbd.brk = (data & 0x80);
                ps2_ports[port].kbd.code = data & 0x7F;
                kbd_keypress(ps2_ports[port].kbd.id, !ps2_ports[port].kbd.brk, (ps2_ports[port].kbd.ext) ? ps2_kbd_scset1_ext[ps2_ports[port].kbd.code] : ps2_kbd_scset1_reg[ps2_ports[port].kbd.code]);
                ps2_ports[port].kbd.ext = 0;
            }
            break;
        case 2: // scancode set 2
            if(data == 0xE0 || data == 0xE1) ps2_ports[port].kbd.ext = data;
            else if(data == 0xF0) ps2_ports[port].kbd.brk = true;
            else {
                ps2_ports[port].kbd.code = data;
                kbd_keypress(ps2_ports[port].kbd.id, !ps2_ports[port].kbd.brk, (ps2_ports[port].kbd.ext) ? ps2_kbd_scset2_ext[ps2_ports[port].kbd.code] : ps2_kbd_scset2_reg[ps2_ports[port].kbd.code]);
                ps2_ports[port].kbd.ext = 0;
                ps2_ports[port].kbd.brk = false;
            }
            break;
        default:
            kerror("keyboard on port %u uses unsupported scancode set %u", port, ps2_ports[port].kbd.scset);
            break;
    }
} 

/* PS/2 DEVICE INCOMING DATA IRQ HANDLER */

static void ps2_irq_handler(uint8_t irq, void* context) {
    (void) context;
    uint8_t port = (irq == PS2_P2_IRQNUM) ? 1 : 0;
    uint8_t data = inb(PS2_IO_DATA); // read data from controller
    // kdebug("PS/2 keyboard driver handling IRQ %u (port %u), data: 0x%02x", irq, port, data);

    if(ps2_ports[port].parse) {
        if(ps2_ports[port].kbd.enabled) ps2_kbd_handler(port, data);
    } else {
        if((ps2_ports[port].data_wridx + 1) % PS2_DATA_BUFLEN == ps2_ports[port].data_rdidx) kwarn("PS/2 port %u buffer is full", port);
        else {
            ps2_ports[port].data[ps2_ports[port].data_wridx] = data;
            ps2_ports[port].data_wridx = (ps2_ports[port].data_wridx + 1) % PS2_DATA_BUFLEN;
        }
    }
}

/* PS/2 DEVICE INCOMING DATA BUFFER ACCESS */

static void ps2_data_reset_buf(uint8_t port) {
    ps2_ports[port].data_rdidx = 0;
    ps2_ports[port].data_wridx = 0;
}

static size_t ps2_data_available(uint8_t port) {
    if(ps2_ports[port].data_wridx >= ps2_ports[port].data_rdidx)
        return (ps2_ports[port].data_wridx - ps2_ports[port].data_rdidx);
    else
        return (PS2_DATA_BUFLEN - ps2_ports[port].data_rdidx + ps2_ports[port].data_wridx);
}

static uint8_t ps2_data_read(uint8_t port) {
    while(ps2_data_available(port) == 0);
    uint8_t ret = ps2_ports[port].data[ps2_ports[port].data_rdidx];
    ps2_ports[port].data_rdidx = (ps2_ports[port].data_rdidx + 1) % PS2_DATA_BUFLEN;
    return ret;
}

static bool ps2_data_read_timeout(uint8_t port, uint8_t* data) {
    timer_tick_t t_start = timer_tick;
    while(timer_tick - t_start < PS2_DATA_TIMEOUT) {
        if(ps2_data_available(port) > 0) break;
    }
    if(ps2_data_available(port) > 0) {
        *data = ps2_data_read(port);
        return true;
    } else return false;
}

/* PS/2 DEVICE DATA TRANSMISSION */

static bool ps2_send_data_ack(uint8_t port, uint8_t data) {
    for(size_t i = 0; i < PS2_DATA_RETRIES; i++) {
        ps2_send_data(port, data);
        uint8_t resp;
        if(!ps2_data_read_timeout(port, &resp)) return false; // timed out waiting for response
        switch(resp) {
            case PS2_DEVRESP_ACK: return true; // command acknowledged
            case PS2_DEVRESP_RESEND: continue; // retry
            default:
                kdebug("unexpected response 0x%02x", resp);
                continue; // TODO
        }
    }
    return false; // comm failure
}

/* PS/2 DEVICE RESET AND IDENTIFICATION */

static bool ps2_post_identify(uint8_t port) {
    if(ps2_ports[port].id == 0xFFFF || ps2_ports[port].id == 0xAB83 || ps2_ports[port].id == 0xAB84 || ps2_ports[port].id == 0xAB85 || ps2_ports[port].id == 0xAB86 || ps2_ports[port].id == 0xAB90 || ps2_ports[port].id == 0xAB91 || ps2_ports[port].id == 0xAB92 || ps2_ports[port].id == 0xACA1) {
        /* keyboard */
        if(ps2_ports[port].kbd.enabled) kbd_unregister(ps2_ports[port].kbd.id);
        ps2_ports[port].kbd.enabled = true;
        ps2_ports[port].kbd.id = kbd_register(NULL); // default keymap
        ps2_ports[port].kbd.brk = false;

        /* check for scancode set override in kernel cmdline */
        char override_key[] = "i8042_p#scs"; override_key[7] = port + '0';
        const char* override_scs = cmdline_find_kvp(override_key);
        if(override_scs != NULL) {
            ps2_ports[port].kbd.scset = strtoul(override_scs, NULL, 10);
            kdebug("port %u keyboard scancode set overridden to %u by kernel cmdline", port, ps2_ports[port].kbd.scset);
        } else {
            if(!ps2_send_data_ack(port, PS2_DEVCMD_KBD_SCSET)) {
                kerror("communication failure on port %u sending PS2_DEVCMD_KBD_SCSET cmd", port);
                return false;
            }
            if(!ps2_send_data_ack(port, 0)) { // get scancode set
                kerror("communication failure on port %u sending PS2_DEVCMD_KBD_SCSET argument", port);
                return false;
            }
            if(!ps2_data_read_timeout(port, &ps2_ports[port].kbd.scset)) {
                kerror("timed out waiting on port %u for scancode set", port);
                return false;
            } else {
                /* handle "translated" results sent by certain hardware such as VMware */
                switch(ps2_ports[port].kbd.scset) {
                    case 0x43: ps2_ports[port].kbd.scset = 1; break;
                    case 0x41: ps2_ports[port].kbd.scset = 2; break;
                    case 0x3F: ps2_ports[port].kbd.scset = 3; break;
                    default: break;
                }
                kdebug("keyboard on port %u uses scancode set %u", port, ps2_ports[port].kbd.scset);
            }
        }
    }

    return true;
}

static bool ps2_reset_device(uint8_t port) {
    bool parse = ps2_ports[port].parse; // save parse status
    ps2_data_reset_buf(port); // discard any pending data
    ps2_send_data(port, PS2_DEVCMD_RESET);
    uint8_t resp_1;
    if(ps2_data_read_timeout(port, &resp_1)) {
        if(resp_1 != 0xFA && resp_1 != 0xAA) {
            kerror("resetting on port %u: unexpected 1st byte 0x%02x", port, resp_1);
            return false;
        }

        uint8_t resp_2;
        if(ps2_data_read_timeout(port, &resp_2)) {
            if(!((resp_1 == 0xFA && resp_2 == 0xAA) || (resp_1 == 0xAA && resp_2 == 0xFA))) {
                kerror("resetting on port %u: unexpected 2nd byte 0x%02x (1st byte 0x%02x)", port, resp_2, resp_1);
                return false;
            }

            /* get ID */
            ps2_ports[port].id = 0;
            if(ps2_data_read_timeout(port, (uint8_t*)&ps2_ports[port].id)) {
                if(ps2_ports[port].id == 0xAB || ps2_ports[port].id == 0xAC) {
                    /* additional ID byte awaits */
                    ps2_ports[port].id <<= 8;
                    if(ps2_data_read_timeout(port, (uint8_t*)&ps2_ports[port].id)) {
                        ps2_ports[port].id = (ps2_ports[port].id << 8) | ps2_data_read(port);
                    } else kwarn("resetting on port %u: timed out waiting for expected 2nd ID byte", port);
                }
            } else {
                kdebug("resetting on port %u: timed out waiting for ID byte - probably keyboard", port);
                ps2_ports[port].id = 0xFFFF;
            }
        } else {
            kerror("resetting on port %u: timed out waiting for 2nd byte", port);
            return false;
        }
    } else {
        kerror("resetting on port %u: timed out waiting for 1st byte", port);
        return false;
    }

    /* check for override in cmdline */
    char override_key[] = "i8042_p#id"; // we'll replace # by the port number here
    override_key[7] = port + '0';
    const char* override_id = cmdline_find_kvp(override_key);
    if(override_id != NULL) {
        ps2_ports[port].id = strtoul(override_id, NULL, 16);
        kdebug("port %u device ID overridden to 0x%04x by kernel cmdline", port, ps2_ports[port].id);
    }

    if(!ps2_post_identify(port)) { // perform post-identification tasks (e.g. device specific setup))
        kerror("resetting on port %u: post-ID tasks failed");
        return false;
    }

    ps2_ports[port].parse = parse; // restore parse status
    return true;
}

/* KERNEL MODULE INITIALIZATION */

int32_t kmod_init(elf_prgload_t* load_result, size_t load_result_len) {
    (void) load_result; (void) load_result_len;
    
    kinfo("Intel 8042 PS/2 controller driver for SysX");

    ps2_ports[0].ok = false; ps2_ports[1].ok = false;
    ps2_ports[0].parse = false; ps2_ports[1].parse = false;
    ps2_ports[0].kbd.enabled = false; ps2_ports[1].kbd.enabled = false;
    
    /* TODO: determine if PS/2 controller exists (via ACPI) */

    kdebug("flushing output buffer");
    inb(PS2_IO_DATA);

    kdebug("performing controller self-test");
    ps2_send_command(PS2_CMD_TEST_CTRLR); // we need to do this first in case it resets the controller
    uint8_t resp = ps2_read_data();
    if(resp != 0x55) {
        kerror("controller self-test failed (response code 0x%02x)", resp);
        return -1;
    }
    
    kdebug("disabling PS/2 ports");
    ps2_send_command(PS2_CMD_DISABLE_P1);
    ps2_send_command(PS2_CMD_DISABLE_P2);

    kdebug("disabling IRQs and translation");
    ps2_write_ccb(ps2_read_ccb() & ~(PS2_CCB_P1_IRQ | PS2_CCB_P2_IRQ | PS2_CCB_P1_TRANS));

    kdebug("enabling PS/2 ports and performing interface tests");
    uint8_t ccb = ps2_read_ccb(); // read CCB and check if any port is out of order (they should all be disabled by now)
    if(ccb & PS2_CCB_P1_CLK) {
        /* 1st port is disabled */
        kdebug("first port is disabled as expected");
        ps2_send_command(PS2_CMD_ENABLE_P1);
        ccb = ps2_read_ccb(); // see how the controller responds
        if(ccb & PS2_CCB_P1_CLK) {
            kwarn("cannot enable first port - malfunction suspected");
            ps2_send_command(PS2_CMD_DISABLE_P1);
        } else {
            kdebug("interface testing first port");
            ps2_send_command(PS2_CMD_TEST_P1);
            resp = ps2_read_data();
            if(resp != 0) {
                kwarn("first port interface testing failed (controller returned %u) - malfunction suspected", resp);
                ps2_send_command(PS2_CMD_DISABLE_P1);
            } else {
                kinfo("first port is functioning");
                ps2_ports[0].ok = true;
            }
        }
    } else kwarn("first port seems to still be enabled - malfunction suspected");
    
    if(ccb & PS2_CCB_P2_CLK) {
        /* 2nd port is disabled */
        kdebug("second port is disabled as expected");
        ps2_send_command(PS2_CMD_ENABLE_P2);
        ccb = ps2_read_ccb(); // see how the controller responds
        if(ccb & PS2_CCB_P2_CLK) {
            kwarn("cannot enable second port - malfunction suspected");
            ps2_send_command(PS2_CMD_DISABLE_P2);
        } else {
            kdebug("interface testing second port");
            ps2_send_command(PS2_CMD_TEST_P2);
            resp = ps2_read_data();
            if(resp != 0) {
                kwarn("second port interface testing failed (controller returned %u) - malfunction suspected", resp);
                ps2_send_command(PS2_CMD_DISABLE_P2);
            } else {
                kinfo("second port is functioning");
                ps2_ports[1].ok = true;
            }
        }
    } else kwarn("second port seems to still be enabled - malfunction suspected");

    kdebug("setting up interrupts");
    ps2_write_ccb(ps2_read_ccb() | ((ps2_ports[0].ok) ? PS2_CCB_P1_IRQ : 0) | ((ps2_ports[1].ok) ? PS2_CCB_P2_IRQ : 0));
    pic_handle(PS2_P1_IRQNUM, &ps2_irq_handler); pic_handle(PS2_P2_IRQNUM, &ps2_irq_handler);
    pic_unmask_bm(((ps2_ports[0].ok) ? (1 << PS2_P1_IRQNUM) : 0) | ((ps2_ports[1].ok) ? (1 << PS2_P2_IRQNUM) : 0));

    for(uint8_t i = 0; i < 2; i++) {
        if(ps2_ports[i].ok) {
            kdebug("resetting device on port %u and retrieving device ID", i);
            if(!ps2_reset_device(i)) {
                kerror("device reset failed");
                // TODO
            } else {
                kdebug("port %u device ID: 0x%04x", i, ps2_ports[i].id);
                ps2_ports[i].parse = true;
            }
        }
    }

    kinfo("PS/2 controller initialized successfully");
    return 0;
}