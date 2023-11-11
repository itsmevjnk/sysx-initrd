#ifndef PS2_IO_H
#define PS2_IO_H

#include <stddef.h>
#include <stdint.h>
#include <arch/x86cpu/asm.h>

/* 8042 IO ports */
#define PS2_IO_DATA         0x60
#define PS2_IO_STATUS       0x64 // read
#define PS2_IO_COMMAND      0x64 // write

/*
 * static inline uint8_t ps2_status()
 *  Returns the PS/2 controller's status register data.
 */
static inline uint8_t ps2_status() {
    return inb(PS2_IO_STATUS);
}

/* PS/2 status register bitmasks */
#define PS2_SR_OB_FULL      (1 << 0) // output buffer is full (data available to be read from PS2_IO_DATA)
#define PS2_SR_IB_FULL      (1 << 1) // input buffer is full (data cannot be written to PS2_IO_DATA or PS2_IO_COMMAND)
#define PS2_SR_SYS          (1 << 2)
#define PS2_SR_CMD_DATA     (1 << 3) // set if data written to input buffer is for PS/2 controller command, cleared if data written is for PS/2 device
#define PS2_SR_TIMEOUT      (1 << 6)
#define PS2_SR_PARITY_ERR   (1 << 7)

/*
 * static inline void ps2_send_command(uint8_t cmd)
 *  Sends a single-byte command to the PS/2 controller.
 */
static inline void ps2_send_command(uint8_t cmd) {
    while(ps2_status() & PS2_SR_IB_FULL);
    outb(PS2_IO_COMMAND, cmd);
}

/* PS/2 commands */
#define PS2_CMD_READ_CCB            0x20 // read Controller Configuration Byte
#define PS2_CMD_WRITE_CCB           0x60 // write Controller Configuration Byte
#define PS2_CMD_DISABLE_P2          0xA7 // disable second port
#define PS2_CMD_ENABLE_P2           0xA8 // enable second port
#define PS2_CMD_TEST_P2             0xA9 // test second port (returns 0 on success)
#define PS2_CMD_TEST_CTRLR          0xAA // test controller (returns 0x55 on success)
#define PS2_CMD_TEST_P1             0xAB // test first port (returns 0 on success)
#define PS2_CMD_DISABLE_P1          0xAD // disable first port
#define PS2_CMD_ENABLE_P1           0xAE // enable first port
#define PS2_CMD_READ_OUTPORT        0xD0 // read Controller Output Port
#define PS2_CMD_WRITE_OUTPORT       0xD1 // write Controller Output Port
#define PS2_CMD_WRITE_P2            0xD4 // write to second port

/* PS/2 CCB bitmasks */
#define PS2_CCB_P1_IRQ              (1 << 0) // first port interrupt
#define PS2_CCB_P2_IRQ              (1 << 1) // second port interrupt
#define PS2_CCB_SYS                 (1 << 2)
#define PS2_CCB_P1_CLK              (1 << 4) // first port clock
#define PS2_CCB_P2_CLK              (1 << 5) // second port clock
#define PS2_CCB_P1_TRANS            (1 << 6) // first port translation

/*
 * static inline void ps2_write_data(uint8_t data)
 *  Writes data to the PS/2 controller.
 */
static inline void ps2_write_data(uint8_t data) {
    while(ps2_status() & PS2_SR_IB_FULL);
    outb(PS2_IO_DATA, data);
}

/*
 * static inline uint8_t ps2_read_data()
 *  Reads data from the PS/2 controller.
 */
static inline uint8_t ps2_read_data() {
    while(!(ps2_status() & PS2_SR_OB_FULL));
    return inb(PS2_IO_DATA);
}

/*
 * static inline uint8_t ps2_read_ccb()
 *  Reads the PS/2 controller's Controller Configuration Byte (CCB).
 */
static inline uint8_t ps2_read_ccb() {
    ps2_send_command(PS2_CMD_READ_CCB);
    return ps2_read_data();
}

/*
 * static inline void ps2_write_ccb(uint8_t ccb)
 *  Writes the PS/2 controller's Controller Configuration Byte (CCB).
 */
static inline void ps2_write_ccb(uint8_t ccb) {
    ps2_send_command(PS2_CMD_WRITE_CCB);
    ps2_write_data(ccb);
}

/*
 * static inline void ps2_send_data(uint8_t port, uint8_t data)
 *  Sends data to the specified PS/2 port.
 */
static inline void ps2_send_data(uint8_t port, uint8_t data) {
    if(port) ps2_send_command(PS2_CMD_WRITE_P2); // write next byte to second port
    ps2_write_data(data);
}

/* PS/2 device commands */
#define PS2_DEVCMD_KBD_SET_LED                  0xED // set LEDs
#define PS2_DEVCMD_KBD_ECHO                     0xEE
#define PS2_DEVCMD_KBD_SCSET                    0xF0 // get/set current scancode set
#define PS2_DEVCMD_IDENTIFY                     0xF2
#define PS2_DEVCMD_KBD_SET_TYPEMATIC            0xF3
#define PS2_DEVCMD_ENABLE_DATA                  0xF4
#define PS2_DEVCMD_DISABLE_DATA                 0xF5
#define PS2_DEVCMD_SET_DEFAULT                  0xF6
#define PS2_DEVCMD_RESEND                       0xFE
#define PS2_DEVCMD_RESET                        0xFF

/* PS/2 device responses */
#define PS2_DEVRESP_BAT_OK                      0xAA // self-test passed
#define PS2_DEVRESP_KBD_ECHO                    0xEE
#define PS2_DEVRESP_ACK                         0xFA
#define PS2_DEVRESP_BAT_FAIL_1                  0xFC
#define PS2_DEVRESP_BAT_FAIL_2                  0xFD
#define PS2_DEVRESP_RESEND                      0xFE

#endif