#include "gate_radio.h"
#include "cc110x_legacy/cc110x-defaultsettings.h"

// OOK, X-tal: 26 MHz (Chip Revision F)
uint8_t gate_radio_config[CC1100_CONF_SIZE] = {
    0x29, // IOCFG2
    0x2E, // IOCFG1
    0x06, // IOCFG0
    0x47, // FIFOTHR
    0xD3, // SYNC1
    0x91, // SYNC0
    sizeof(gate_toggle_code), // PKTLEN
    0x04, // PKTCTRL1
    0x00, // PKTCTRL0   (variable packet length)
    0x00, // ADDR
    0x00, // CHANNR
    0x06, // FSCTRL1
    0x00, // FSCTRL0
    0x10, // FREQ2
    0xB1, // FREQ1
    0x3B, // FREQ0
    0xF6, // MDMCFG4
    0xE4, // MDMCFG3
    0x30, // MDMCFG2
    0x22, // MDMCFG1
    0xF8, // MDMCFG0
    0x15, // DEVIATN
    0x07, // MCSM2
    0x49, // MCSM1
    0x10, // MCSM0
    0x16, // FOCCFG
    0x6C, // BSCFG
    0x03, // AGCCTRL2
    0x00, // AGCCTRL1
    0x91, // AGCCTRL0
    0x80, // WOREVT1
    0x00, // WOREVT0
    0xF0, // WORCTRL
    0x56, // FREND1
    0x11, // FREND0
    0xE9, // FSCAL3
    0x2A, // FSCAL2
    0x00, // FSCAL1
    0x1F, // FSCAL0
    0x00  // padding to 4 bytes
};