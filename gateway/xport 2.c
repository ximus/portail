/**
 * Why the posix interface?
 * No particular reason. If it's little effort, might as well
 * provide a more standard interface.
 */

#include <stdio.h>
#include <errno.h>

#include "kernel.h"
#include "irq.h"
#include "thread.h"
#include "msg.h"
#include "circular_buffer.h"

#include "uart_block.h"

#include "xport.h"


/* increase stack size in uart0 when setting this to 1 */
#define ENABLE_DEBUG    (0)
#include "debug.h"


#define RX_BUF_LEN        (2)
#define FRAME_DELIMITER   (0x00)

volatile kernel_pid_t xport_pid = KERNEL_PID_UNDEF;

/* thread stack */
static char stack_buffer[KERNEL_CONF_STACKSIZE_DEFAULT];

struct {
    enum {
        DELIMIT,    /* frame delimiter */
        LENGTH,     /* data length */
        DATA,       /* variable length, cobs encoded */
        CRC_H,      /* 16 bit CRC high byte */
        CRC_L,      /* 16 bit CRC low byte */
        COMPLETE
    } state;
    /**
     * put simply a queue of length 1 pointing to the last
     * packet received
     */
    xport_buffer_t *rdy_pkt;
} parser;

struct {
    kernel_pid_t pid = KERNEL_PID_UNDEF;
    uint8_t *buffer;
} reader;

static xport_packet_t rx_buffer[XPORT_RX_BUF_LEN];
static uint8_t rx_buffer_pos = 0;


void xport_init(void) {
    uart_init();
    // uart set byte rx callback
    xport_pid = thread_create(
      stack_buffer,
      sizeof(stack_buffer),
      PRIORITY_MAIN - 2,
      CREATE_STACKTEST | CREATE_SLEEPING,
      thread_loop,
      NULL,
      "xport"
    );
}

static void handle_incoming_byte(uint8_t byte)
{
    xport_packet_t *pkt = &pkt_rx_buffer[rx_buffer_pos];

    switch (parser.state) {
        case DELIMIT:
            if (byte == FRAME_DELIMITER)
            {
                parser.state = LENGTH;
            }
            break;
        case LENGTH:
            parser.len = byte;
            parser.state = DATA;
            uart_block_receive(pkt->data, parser.len, on_data_complete);
            break;
        case DATA:
            // never reaches here, data transfers via DMA
            break;
        case CRC_H:
            pkt->crc |= byte << 8;
            parser.state = CRC_H;
            break;
        case CRC_L:
            pkt->crc |= byte;
            parser.state = COMPLETE;
            // cheat, no break, go straight to complete
        case COMPLETE:
            parser.rdy_pkt = pkt;
            rx_buffer_pos++;
            rx_buffer_pos %= sizeof(rx_buffer);
            msg_send_int(xport_pid);
            break;
    }
}

static void on_data_complete(void)
{
    parser.state = CRC_H;
}

static int min(int a, int b)
{
    if (b > a) {
        return a;
    }
    else {
        return b;
    }
}

static drop_packet(void*)
{
    // noop, would be nice to blink a red led
}

static void thread_loop(void *arg)
{
    (void) arg;
    msg_t m;

    reader.pid = KERNEL_PID_UNDEF;
    reader.iop = NULL;

    while (1) {
        msg_receive(&m);

        /* sent by another thread? as opposed to an internal interrupt */
        if (!msg_sent_by_int(&m))
        {
            switch(m.type) {
                case OPEN:
                    // don't check read ownership, this app is small enough
                    reader.pid = msg.sender_pid;
                    reader.iop = (struct posix_iop_t *) msg.content.ptr;
                    break;
                case WRITE:
                    break;
            }
        }

        /* is data available and do we have reader waiting? */
        if (parser.rdy_pkt != NULL && reader.iop != NULL)
        {
            unsigned       state = disableIRQ();
            xport_packet_t *pkt  = parser.rdy_pkt;
            posix_iop_t    *r    = reader.iop;

            uint8_t  len = cobs_decode(pkt->data, pkt->len, r->buffer);
            uint16_t crc = crc16(r, len);

            if (crc == pkt->crc)
            {
                m.sender_pid = reader.pid;
                m.type = READ;
                m.content.value = (char *) r;

                msg_reply(&m, &m);

                reader.iop = NULL;
            }
            else {
                drop_packet();
            }

            parser.rdy_pkt = NULL;
            restoreIRQ(state);
        }
    }
}

void xport_register(kernel_pid_t pid)
{
    reader_pid = pid;
}