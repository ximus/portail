// https://tools.ietf.org/html/draft-shelby-core-link-format-00

#include "leds.h"
#include "thread.h"
#include "transceiver.h"
#include "coap.h"


#define ENABLE_DEBUG (1)
#include "debug.h"

#define TRANCSCEIVER TRANSCEIVER_DEFAULT

// i don't understand the reasoning behind this buffer
#define SCRATCH_BUF_LEN (2)
uint8_t scratch_buf_buf[SCRATCH_BUF_LEN];
coap_rw_buffer_t scratch_buf = {scratch_buf_buf, SCRATCH_BUF_LEN};

#include "endpoints.c"

#define MSG_BUFFER_SIZE        (32)
#define ENDPOINT_STACK_SIZE    (THREAD_STACKSIZE_DEFAULT)

static char  stack_buffer[ENDPOINT_STACK_SIZE];
static msg_t msg_q[MSG_BUFFER_SIZE];

static struct {
  uint radio_buffer_full;
  uint radio_send_fail;
  uint coap_parse_error;
} stats;

kernel_pid_t api_endpoint_pid = KERNEL_PID_UNDEF;


void *api_thread(void *arg)
{
    (void) arg;

    msg_t m;

    radio_packet_t *radio_req;
    radio_packet_t radio_resp;

    coap_packet_t coap_req;
    coap_packet_t coap_resp;

    transceiver_command_t tcmd;

    tcmd.transceivers = TRANCSCEIVER;

    // do i need this queue?
    msg_init_queue(msg_q, MSG_BUFFER_SIZE);

    transceiver_init(TRANCSCEIVER);
    transceiver_start();
    transceiver_register(TRANCSCEIVER, thread_getpid());

    well_known_core_build(endpoints);
    LED_YELLOW_ON;

    while (1)
    {
        msg_receive(&m);

        LED_GREEN_ON;

        if (m.type == PKT_PENDING) {
            radio_req = (radio_packet_t *) m.content.ptr;

            uint8_t resp_len = 0;
            int err = coap_parse(&coap_req, radio_req->data, radio_req->length);
            if (err) {
                stats.coap_parse_error += 1;
                DEBUG("Bad packet err=%d\n", err);
                // silent packet drop
            }
            else {
                resp_len = coap_handle_req(&scratch_buf, &coap_req, &coap_resp);

                // send response to sender
                radio_resp.dst = radio_req->src;
                radio_resp.data = coap_resp;
                radio_resp.length = resp_len;

                tcmd.data = &radio_resp;
                m.type = SND_PKT;
                m.content.ptr = (char *) &tcmd;
                msg_send_receive(&m, &m, transceiver_pid);

                int8_t sent_len = (int8_t) m.content.value;
                if (sent_len == 0) {
                    stats.radio_send_fail += 1;
                }
            }

            // release the packet
            radio_pkt->processing--;
        }
        else if (m.type == ENOBUFFER) {
            stats.radio_buffer_full += 1;
            DEBUG("Transceiver buffer full");
        }

        LED_GREEN_OFF;
    }
}

void api_start(void)
{
    api_endpoint_pid = thread_create(
        stack_buffer,
        sizeof(stack_buffer),
        THREAD_PRIORITY_MAIN - 2,
        CREATE_STACKTEST,
        api_thread,
        NULL,
        "api"
    );
}