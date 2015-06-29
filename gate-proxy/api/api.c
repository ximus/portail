// https://tools.ietf.org/html/draft-shelby-core-link-format-00

#include "leds.h"
#include "thread.h"
#include "hwtimer.h"
#include "coap.h"
#include "net_layer.h"
#include "unistd.h"
#include "api.h"
#include "portail.h"

#include "endpoints.h"


#define ENABLE_DEBUG (1)
#include "debug.h"

#define TRANCSCEIVER TRANSCEIVER_DEFAULT

// i don't understand the reasoning behind this buffer
#define SCRATCH_BUF_LEN (2)
uint8_t scratch_buf_buf[SCRATCH_BUF_LEN];
coap_rw_buffer_t scratch_buf = {scratch_buf_buf, SCRATCH_BUF_LEN};

#define RESP_BUFFER_LEN PORTAIL_MAX_DATA_SIZE
uint8_t resp_buffer[RESP_BUFFER_LEN];

#define MSG_BUFFER_SIZE        (8) /* pow of two */
#define ENDPOINT_STACK_SIZE    (THREAD_STACKSIZE_DEFAULT)

static char  stack_buffer[ENDPOINT_STACK_SIZE];
static msg_t msg_q[MSG_BUFFER_SIZE];

static struct {
  uint radio_buffer_full;
  uint resp_send_fail;
  uint coap_parse_error;
  uint coap_build_error;
  uint coap_endpoint_error;
} stats;

static int sock = SOCK_UNDEF;

kernel_pid_t api_endpoint_pid = KERNEL_PID_UNDEF;


static void handle_req(char *buffer, int length, uint dest_port)
{
    static coap_packet_t coap_req;
    static coap_packet_t coap_resp;

    /* parse coap request */
    int err = coap_parse(&coap_req, (uint8_t *) buffer, length);
    if (err) {
        stats.coap_parse_error += 1;
        DEBUG("Bad packet err=%d\n", err);
        /* silent packet drop */
        return;
    }

    /* handle request */
    err = coap_handle_req(&scratch_buf, &coap_req, &coap_resp);
    if (err) {
        stats.coap_endpoint_error += 1;
        DEBUG("Endpoint failed err=%d\n", err);
        /* silent packet drop */
        return;
    }

    /* build coap response*/
    size_t resp_len = RESP_BUFFER_LEN;
    err = coap_build(resp_buffer, &resp_len, &coap_resp);
    if (err) {
        stats.coap_build_error += 1;
        DEBUG("Bad response err=%d\n", err);
        /* silent packet drop */
        return;
    }

    /* send response back over the air */
    int len = netl_send_to(sock, resp_buffer, resp_len, dest_port);

    if (len == -1)
    {
        DEBUG("api: failed to send response");
        stats.resp_send_fail += 1;
    }
}

void *api_thread(void *arg)
{
    (void) arg;

    msg_t m;

    msg_init_queue(msg_q, MSG_BUFFER_SIZE);

    well_known_core_build(endpoints);

    /* keep trying for socket */
    while (sock == SOCK_UNDEF) {
        sock = netl_socket(SOCK_DATA);
        DEBUG("api: cannot get socket");
        sleep(1);
    }

    netl_bind(sock, API_UDP_PORT);

    LED_YELLOW_ON;

    static char buffer[PORTAIL_MAX_DATA_SIZE];
    uint src_port;

    while (1)
    {
        int len = netl_recv(sock, buffer, sizeof(buffer), &src_port);

        if (0 < len)
        {
            LED_GREEN_ON;

            handle_req(buffer, len, src_port);

            LED_GREEN_OFF;
        }
    }
}

void api_start(void)
{
    netl_init();

    sock = netl_socket(SOCK_DATA);

    if (api_endpoint_pid == KERNEL_PID_UNDEF)
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
}