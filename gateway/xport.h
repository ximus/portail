#include "cc110x_legacy.h"

/**
 * While writing this module's api, I was quite new to writing c
 * and embeded software. I think it could use maturation, maybe
 * implement RIOT's posix api, but I walked a thin line of trying
 * to balance efficiency (like avoid memcpy) and readablity/complexity.
 * I'm sure significant refactoring could be done, beyond the fact that
 * it this whole app probably doesn't mandate riot and should be
 * bare metal.
 */

#define XPORT_MAX_PACKET_SIZE   (CC1100_MAX_DATA_LENGTH)

extern volatile kernel_pid_t xport_pid;

// /* thread msg */
// enum xport_msg_type_t {
//     /**
//      * pass pointer to data buffer through msg content.ptr
//      * length of data written returned through msg content.value
//      * warning: there is no upper bound on the amount
//      */
//     XPORT_READ
// };

/**
 * internally this can represent either encoded or decoded packets
 * externally this only represents decoded packets.
 */
typedef struct {
  uint8_t  len;
  uint8_t  data[XPORT_MAX_PACKET_SIZE];
  uint16_t crc;
} xport_packet_t;


void xport_init(void);
void xport_register(kernel_pid_t);