#include <thread.h>
#include <transceiver.h>
#include "xport.h"
#include "relay.h"
#include "posix_io.h"

// TODO: check priorities

#define DEBUG 1

#define RELAY_STACK_SIZE      (THREAD_STACKSIZE_DEFAULT)
#define RCV_BUFFER_SIZE       (64)

static char stack_buffer[RELAY_STACK_SIZE];
static msg_t msg_q[RCV_BUFFER_SIZE];

volatile kernel_pid_t relay_pid = KERNEL_PID_UNDEF;

// static void handle_tranceiver_msg(*msg_t);
static void handle_xport_msg(msg_t*);

void *relay_thread(void*);

// expects a transceiver initialized and started
// expects xport initialized
void relay_start(void)
{
    relay_pid = thread_create(
      stack_buffer,
      sizeof(stack_buffer),
      THREAD_PRIORITY_MAIN - 2,
      CREATE_STACKTEST,
      relay_thread,
      NULL,
      "relay"
    );

    // transceiver_register(TRANSCEIVER_DEFAULT, relay_pid);
}

void *relay_thread(void *arg)
{
    (void) arg;

    // msg_t m;

    msg_init_queue(msg_q, RCV_BUFFER_SIZE);

    while (1)
    {
        static char buffer[100];
        int len;

        len = posix_read(xport_pid, buffer, sizeof(buffer));
        posix_write(xport_pid, buffer, len);
    }
}

// void *handle_tranceiver_msg(msg_t *m) {
//     if (m->type == PKT_PENDING) {
//         p = (radio_packet_t *) m->content.ptr;

//         // LED YELLOW ON
//         posix_write(xport_pid, p->data, p->length);
//         // LED YELLOW OFF

//         // release the packet
//         p->processing--;
//         // puts("\n");

//     }
//     else if (m->type == ENOBUFFER) {
//         puts("Transceiver buffer full");
//     }
//     else {
//         puts("Unknown packet received");
//     }
// }


/**
 * From a software design point of view, COBS decoding and CRC checking
 * should be encapsulated in the the xport.c module. Putting it here
 * is a tradeoff that allows for one less thread (no need for xport.c to
 * do any work outside of interrupts) and lower memory usage (COBS decoding
 * requires a destination memory block, relay.c set that to be the final
 * radio packet rather than an extra intermidary block).
 */
// static void data_to_radio(uint8_t *data, uint8_t len)
// {
//     msg_t m;
//     radio_packet_t radio_packet;

//     radio_packet.length = xport_packet->length;
//     radio_packet.data   = xport_packet->data;

//     m.type = SND_PKT;
//     m.content.ptr = (char *) &radio_packet;

//     msg_send_receive(&m, &m, radio_pid);
// }