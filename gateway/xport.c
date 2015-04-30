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

#include "uart_block.h"

#include "xport.h"


/* increase stack size in uart0 when setting this to 1 */
#define ENABLE_DEBUG    (0)
#include "debug.h"


#define RX_BUF_LEN        (2)
#define FRAME_DELIMITER   (0x00)
#define COBS_MAX_OVERHEAD (1) // bytes
#define DATA_MAX_SIZE     (CC1100_MAX_DATA_LENGTH + COBS_MAX_OVERHEAD)

volatile kernel_pid_t xport_pid = KERNEL_PID_UNDEF;

/* thread stack */
static char stack_buffer[KERNEL_CONF_STACKSIZE_DEFAULT];

typedef struct {
  uint8_t  len;
  uint8_t  data[DATA_MAX_SIZE];
  uint16_t crc;
} packet_t;

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
    packet_t *rdy_pkt;
} parser;

static struct {
    kernel_pid_t pid = KERNEL_PID_UNDEF;
    posix_iop_t *iop;
} reader;

static struct {
    kernel_pid_t pid = KERNEL_PID_UNDEF;
    posix_iop_t *iop;
} writer;

static packet_t rx_buffer[RX_BUF_LEN];
static uint8_t  rx_buffer_pos = 0;

/* holds just one packet */
static uint8_t tx_buffer[sizeof(packet_t)];


void xport_init(void) {
    uart_init();
    reset_parser();
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

static void reset_parser(void)
{
    parser.state = DELIMIT;
}

static void handle_incoming_byte(uint8_t byte)
{
    packet_t *pkt = &rx_buffer[rx_buffer_pos];

    switch (parser.state) {
        case DELIMIT:
            if (byte == FRAME_DELIMITER)
            {
                parser.state = LENGTH;
            }
            break;
        case LENGTH:
            if (byte <= DATA_MAX_SIZE)
            {
                pkt->len = byte;
                parser.state = DATA;
                uart_block_receive(pkt->data, pkt->len, on_data_complete);
            }
            else {
                reset_parser();
            }
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

static void on_packet_dropped(void)
{
    /**
     * it would be nice to blink a red led or something
     * or maybe keep stats and send them over network
     * when a special request comes in
     */
}

static void on_send_complete(void)
{
    if (writer.iop != NULL)
    {
        static msg_t m;

        m.sender_pid = writer.pid;
        m.type = WRITE;
        m.content.value = (char *) writer.iop;

        msg_reply_int(&m, &m);
    }
}

uint8_t make_pkt(uint8_t *data, size_t len, uint8_t *dest)
{
    if (len + COBS_MAX_OVERHEAD > DATA_MAX_SIZE)
    {
        return 0;
    }
    /* first need to crc and encode the msg */
    uint16_t crc = crc16(data, len);
    /* encoded msg is written directly to its dest offset */
    uint8_t enc_msg_len = cobs_encode(data, len, &dest[2]);

    int write_offset = 0;

    /* packet starts with a marker */
    dest[write_offset++] = PKT_DELIMITER;

    /* next is the 1 byte length of data */
    dest[write_offset++] = enc_msg_len;

    /* next is the encoded data, already written */
    write_offset += enc_msg_len;

    /* last is the 2 byte crc */
    dest[write_offset++] = (crc >> 8) & 0xFF;
    dest[write_offset++] = crc & 0xFF;

    return write_offset;
}

static void thread_loop(void *arg)
{
    (void) arg;
    msg_t m;

    reader.pid = KERNEL_PID_UNDEF;
    reader.iop = NULL;
    struct posix_iop_t *iop = NULL;

    while (1)
    {
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
                    iop = (struct posix_iop_t *) msg.content.ptr;
                    uint8_t pkt_len = make_pkt(
                        iop->buffer, iop->nbytes, tx_buffer
                    );
                    if (pkt_len > 0) {
                        uart_dma_send(tx_buffer, pkt_len, on_send_complete);
                    }
                    else {
                        on_packet_dropped();
                    }
                    break;
            }
        }

        /* is data available and do we have reader waiting? */
        if (parser.rdy_pkt != NULL && reader.iop != NULL)
        {
            unsigned    state = disableIRQ();
            packet_t    *pkt  = parser.rdy_pkt;
            posix_iop_t *r    = reader.iop;

            uint8_t  len = cobs_decode(pkt->data, pkt->len, r->buffer, r->nbytes);
            uint16_t crc = crc16(r->buffer, len);

            if (crc == pkt->crc)
            {
                m.sender_pid = reader.pid;
                m.type = READ;
                m.content.value = (char *) r;

                msg_reply(&m, &m);

                reader.iop = NULL;
            }
            else {
                on_packet_dropped();
            }

            parser.rdy_pkt = NULL;
            restoreIRQ(state);
        }
    }
}