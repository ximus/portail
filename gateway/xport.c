/**
 * Character device messaging loop.
 *
 * Copyright (C) 2013, INRIA.
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 * @ingroup chardev
 * @{
 * @file    chardev_thread.c
 * @brief   Runs an infinite loop in a separate thread to handle access to character devices such as uart.
 * @author  Kaspar Schleiser <kaspar@schleiser.de>
 * @}
 */

#include <stdio.h>
#include <errno.h>

#include "kernel.h"
#include "irq.h"

#include "thread.h"
#include "msg.h"

#include "gpioint.h"
#include "uart_block.h"

#include "xport.h"


/* increase stack size in uart0 when setting this to 1 */
#define ENABLE_DEBUG    (0)
#include "debug.h"

 // #define RELAY_STACK_SIZE      (KERNEL_CONF_STACKSIZE_DEFAULT)
 // #define RCV_BUFFER_SIZE       (64)

 // static char stack_buffer[RELAY_STACK_SIZE];
 // static msg_t msg_q[RCV_BUFFER_SIZE];


// #define DTR_PORT       (2)
// #define DTR_PORT_NUM   BIT3
// inline uint16_t DTR_get(void) { return P2IN & DTR_PORT_NUM; }

volatile kernel_pid_t xport_pid  = KERNEL_PID_UNDEF;
static kernel_pid_t   reader_pid = KERNEL_PID_UNDEF;


static xport_packet_t rcv_buffer[XPORT_RCV_BUFSIZE];
static uint8_t        rcv_buffer_pos = 0;

static int init_DTR_port(void);

void xport_init(void) {
    // xport_pid = thread_create(
    //                   stack_buffer,
    //                   sizeof(stack_buffer),
    //                   PRIORITY_MAIN - 2,
    //                   CREATE_STACKTEST,
    //                   device_loop,
    //                   NULL,
    //                   "xport");
    init_DTR_port();
}

// static void DTR_handler(void);

static void frame_start_handler(void);
static void frame_end_handler(void);

static int init_DTR_port(void)
{
    gpioint_set(
        1,
        BIT2,
        GPIOINT_RISING_EDGE,
        frame_start_handler
    );
    gpioint_set(
        1,
        BIT3,
        GPIOINT_FALLING_EDGE,
        frame_end_handler
    );

    return 0;
}

static void frame_start_handler(void)
{
    P3OUT |= BIT7;
    uart_block_receive(rcv_buffer[rcv_buffer_pos].data, XPORT_MAX_PACKET_SIZE, NULL);
}

static inline void try_notify_reader(void);

static void frame_end_handler(void)
{
    P3OUT &= ~BIT7;
    int state = disableIRQ();
    uint8_t received_count = uart_block_received_count();
    uart_block_receive_end();
    restoreIRQ(state);

    rcv_buffer[rcv_buffer_pos].length = received_count;

    try_notify_reader();

    if (++rcv_buffer_pos == XPORT_RCV_BUFSIZE)
    {
        rcv_buffer_pos = 0;
    }
}

static inline void try_notify_reader(void)
{
    if (reader_pid != KERNEL_PID_UNDEF)
    {
        static msg_t m;
        m.type        = PKT_PENDING;
        m.content.ptr = (char *) &rcv_buffer[rcv_buffer_pos];
        msg_send_int(&m, reader_pid);
    }
}


// static void DTR_handler(void)
// {
//     if (DTR_get())
//     {
//         frame_start_handler();
//     }
//     else {
//         frame_end_handler();
//     }
// }

void xport_register(kernel_pid_t pid)
{
    reader_pid = pid;
}


// void *chardev_thread_entry(void *rb_)
// {
//     ringbuffer_t *rb = rb_;
//     chardev_loop(rb);
//     return NULL;
// }

// // could be abstracted in to general block device, the likes of chardev
// // not sure how useful it would be
// static void device_loop(ringbuffer_t *rb)
// {
//     msg_t m;

//     kernel_pid_t pid = thread_getpid();

//     // ringbuffer_init(&xport_ringbuffer, buffer, XPORT_BUFSIZE);

//     kernel_pid_t reader_pid = KERNEL_PID_UNDEF;
//     struct posix_iop_t *r = NULL;

//     puts("XPort thread started.");

//     while (1) {
//         msg_receive(&m);

//         if (m.sender_pid != pid) {
//             DEBUG("Receiving message from another thread: ");

//             switch(m.type) {
//                 case PACKET_RECEIVED:
//                     // here
//                     break;
//                 case OPEN:
//                     DEBUG("OPEN\n");
//                     if (reader_pid == KERNEL_PID_UNDEF) {
//                         reader_pid = m.sender_pid;
//                         /* no error */
//                         m.content.value = 0;
//                     }
//                     else {
//                         m.content.value = -EBUSY;
//                     }

//                     msg_reply(&m, &m);
//                     break;

//                 case CLOSE:
//                     DEBUG("CLOSE\n");
//                     if (m.sender_pid == reader_pid) {
//                         DEBUG("xport_thread: closing file from %" PRIkernel_pid "\n", reader_pid);
//                         reader_pid = KERNEL_PID_UNDEF;
//                         r = NULL;
//                         m.content.value = 0;
//                     }
//                     else {
//                         m.content.value = -EINVAL;
//                     }

//                     msg_reply(&m, &m);
//                     break;

//                 default:
//                     DEBUG("UNKNOWN\n");
//                     m.content.value = -EINVAL;
//                     msg_reply(&m, &m);
//             }
//         }

//         if (num_bytes_received > 0 && (r != NULL)) {
//             DEBUG("Data is available\n");
//             int state = disableIRQ();
//             int nbytes = num_bytes_received;
//             DEBUG("xport_thread [%i]: sending %i bytes received from %" PRIkernel_pid " to pid %" PRIkernel_pid "\n", pid, nbytes, m.sender_pid, reader_pid);
//             ringbuffer_get(rb, r->buffer, nbytes);
//             r->nbytes = nbytes;

//             m.sender_pid = reader_pid;
//             m.type = OPEN;
//             m.content.ptr = (char *)r;

//             msg_reply(&m, &m);

//             r = NULL;
//             restoreIRQ(state);
//         }
//     }
// }
