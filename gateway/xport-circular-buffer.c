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

// #include <stdio.h>
// #include <errno.h>

// #include "kernel.h"
// #include "irq.h"

// #include "thread.h"
// #include "msg.h"

// #include "ringbuffer.h"
// #include "posix_io.h"

// #include "gpioinit.h"
// #include "uart_block.h"


// /* increase stack size in uart0 when setting this to 1 */
// #define ENABLE_DEBUG    (0)
// #include "debug.h"

// // TODO(ximus): ideally max packet size should be specified by clients,
// // through the posix api somehow. File descriptor?
// #define MAX_PACKET_SIZE      (CC1100_MAX_DATA_LENGTH)
// #define XPORT_RCV_BUFSIZE    (MAX_PACKET_SIZE * 3)

// const DTR_PORT      = 1
// const DTR_PORT_NUM  = BIT2


// kernel_pid_t        xport_pid  = KERNEL_PID_UNDEF;
// static kernel_pid_t reader_pid = KERNEL_PID_UNDEF;



// static volatile uint8_t rcv_buffer[XPORT_RCV_BUFSIZE];
// static int rcv_buffer_end = rcv_buffer + XPORT_RCV_BUFSIZE;
// static uint8_t *rcv_buffer_pos = rcv_buffer;


// void xport_init(void) {
//     if (xport_pid != KERNEL_PID_UNDEF) {
//         /* do not re-initialize */
//         return;
//     }

//     xport_pid = thread_create(
//                       radio_stack_buffer,
//                       sizeof(radio_stack_buffer),
//                       PRIORITY_MAIN - 2,
//                       CREATE_STACKTEST,
//                       device_loop,
//                       NULL,
//                       "xport");
//     xport_init_DTR_port();
// }

// int xport_init_DTR_port(uint8_t port, uint8_t port_mask)
// {
//     return gpioint_set(port, port_mask, GPIOINT_FALLING_EDGE | GPIOINT_RISING_EDGE, DTR_handler);
// }

// static void DTR_on_handler(void)
// {
//     uart_block_receive(&rcv_buffer_pos, XPORT_RCV_BUFSIZE);
// }

// static void DTR_off_handler(void)
// {
//     int state = disableIRQ();
//     uint8_t num_bytes_received = uart_block_received_count();
//     uart_block_receive_end();
//     rcv_buffer_observe_used(num_bytes_received);
//     restoreIRQ(state);

//     if (!read_pending) {
//         static msg_t m;
//         m.type = PKT_PENDING;
//         msg_send_int(&m, xport_pid);
//     }
// }


// // There is an issue with this buffer algorithm:
// // say the receiving thread is not able to run
// static inline rcv_buffer_observe_used(uint8_t num_bytes_used)
// {
//     // make sure the next postion allows for a packet without overflowing
//     int projected_pos = rcv_buffer_pos + num_bytes_used;
//     if (projected_pos >= rcv_buffer_end - MAX_PACKET_SIZE)
//     {
//         // can overflow, back to start of buffer
//         rcv_buffer_pos = rcv_buffer;
//     }
//     else {
//         // cannot overflow
//         rcv_buffer_pos = projected_pos;
//     }
// }

// static void DTR_handler()
// {
//     if (P)
//     {

//     }
// }

// void xport_register(kernel_pid_t pid)
// {
//     reader_id = pid;
// }

// uint8_t xport_read()
// {

// }

// int xport_send()
// {

// }


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
