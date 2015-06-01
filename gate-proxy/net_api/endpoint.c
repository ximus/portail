// https://tools.ietf.org/html/draft-shelby-core-link-format-00

#include "thread.h"
#include "transceiver.h"
#include "coap.h"
#include "circular_buffer.h"

// #include "netdev/base.h"
// #include "netdev/default.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

// i don't understand the reasoning behind this buffer
#define SCRATCH_BUF_LEN (2)
uint8_t scratch_buf_buf[SCRATCH_BUF_LEN];
coap_rw_buffer_t scratch_buf = {scratch_buf_buf, SCRATCH_BUF_LEN};

#include "endpoints.c"

#define MSG_BUFFER_SIZE        (32)
#define ENDPOINT_STACK_SIZE    (THREAD_STACKSIZE_DEFAULT)

static char  stack_buffer[ENDPOINT_STACK_SIZE];
static msg_t msg_q[MSG_BUFFER_SIZE];

kernel_pid_t api_endpoint_pid = KERNEL_PID_UNDEF;

// #define PKT_RCV_BUFFER_LEN  (256)
// static uint8_t pkt_rvc_buffer[PKT_RCV_BUFFER_LEN]
// static circ_buffer_t pkt_rcv_circ_buffer = CIRC_BUFFER_INIT(PKT_RCV_BUFFER_LEN);
// static netdev_t *netdev;


void *api_endpoint(void *arg)
{
    (void) arg;

    msg_t m;

    radio_packet_t *radio_pkt;
    coap_packet_t coap_rcv_pkt;
    coap_packet_t coap_resp_pkt;


    msg_init_queue(msg_q, MSG_BUFFER_SIZE);

    transceiver_register(TRANSCEIVER_DEFAULT, thread_getpid());

    well_known_core_build(endpoints);

    while (1)
    {
        msg_receive(&m);

        if (m.type == PKT_PENDING) {
            radio_pkt = (radio_packet_t *) m.content.ptr;

            int err = coap_parse(&coap_rcv_pkt, radio_pkt->data, radio_pkt->length);
            if (err) {
                printf("Bad packet err=%d\n", err);
            }
            else {
                coap_handle_req(&scratch_buf, &coap_rcv_pkt, &coap_resp_pkt);
            }

            // send response to sender
            radio_pkt->dst = radio_pkt->src;
            m.type = SND_PKT;
            // LED ON
            msg_send_receive(&m, &m, transceiver_pid);
            // LED OFF

            // printf("Got api_endpoint packet:\n");
            // printf("\tLength:\t%u\n", p->length);
            // printf("\tSrc:\t%u\n", p->src);
            // printf("\tDst:\t%u\n", p->dst);
            // printf("\tLQI:\t%u\n", p->lqi);
            // printf("\tRSSI:\t%u\n", p->rssi);

            // for (i = 0; i < p->length; i++) {
            //     printf("%02X ", p->data[i]);
            // }

            // release the packet
            radio_pkt->processing--;
            // puts("\n");
        }
        else if (m.type == ENOBUFFER) {
            DEBUG("Transceiver buffer full");
        }
        else {
            DEBUG("Unknown packet received");
        }
    }
}

// int handle_net_packet(
//     netdev_t *dev,
//     void *src,
//     size_t src_len,
//     void *dest,
//     size_t dest_len,
//     void *payload,
//     size_t payload_len)
// {
//     (void) dev;
//     (void) dest;
//     (void) dest_len;

//     int index = circ_buffer_add(pkt_rcv_circ_buffer);
//     // pkt_rvc_buffer[index];
// }

void api_endpoint_start(void)
{
    // netdev = NETDEV_DEFAULT;

    // netdev->driver->add_receive_data_callback();

    api_endpoint_pid = thread_create(
        stack_buffer,
        sizeof(stack_buffer),
        THREAD_PRIORITY_MAIN - 2,
        CREATE_STACKTEST,
        api_endpoint,
        NULL,
        "api_endpoint"
    );
}