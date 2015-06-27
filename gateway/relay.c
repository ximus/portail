#include <string.h>

#include "thread.h"
#include "transceiver.h"
#include "posix_io.h"

#include "portail.h"
#include "xport.h"
#include "relay.h"
#include "leds.h"
#include "hwtimer.h"


#define ENABLE_DEBUG    (1)
#include "debug.h"

static char uart_to_radio_stack[THREAD_STACKSIZE_DEFAULT];
static char radio_to_uart_stack[THREAD_STACKSIZE_DEFAULT];

#define TRANCSCEIVER TRANSCEIVER_DEFAULT

volatile kernel_pid_t uart_to_radio_pid = KERNEL_PID_UNDEF;
volatile kernel_pid_t radio_to_uart_pid = KERNEL_PID_UNDEF;

static struct {
  uint radio_buffer_full;
  uint radio_send_fail;
} stats;


void *uart_to_radio_thread(void*);
void *radio_to_uart_thread(void*);

void relay_start(void)
{
    transceiver_init(TRANCSCEIVER);
    transceiver_start();

    xport_init();

    uart_to_radio_pid = thread_create(
      uart_to_radio_stack,
      sizeof(uart_to_radio_stack),
      THREAD_PRIORITY_MAIN - 2,
      CREATE_STACKTEST,
      uart_to_radio_thread,
      NULL,
      "uart_to_radio"
    );

    radio_to_uart_pid = thread_create(
      radio_to_uart_stack,
      sizeof(radio_to_uart_stack),
      THREAD_PRIORITY_MAIN - 2,
      CREATE_STACKTEST,
      radio_to_uart_thread,
      NULL,
      "radio_to_uart"
    );
}

void *uart_to_radio_thread(void *arg)
{
    (void) arg;

    msg_t m;
    static radio_packet_t radio_pkt;
    static transceiver_command_t tcmd;
    static uint8_t buffer[PORTAIL_MAX_DATA_SIZE];

    radio_pkt.data = buffer;
    radio_pkt.dst = TRANSCEIVER_BROADCAST;

    tcmd.transceivers = TRANCSCEIVER;
    tcmd.data = &radio_pkt;

    while (1)
    {
        DEBUG("uart_to_radio: waiting ...\n");

        int8_t len = posix_read(xport_pid, buffer, sizeof(buffer));
        radio_pkt.length = len;

        DEBUG("uart_to_radio: relaying %d bytes\n", len);
        LED_GREEN_ON;

        m.type = SND_PKT;
        m.content.ptr = (char *) &tcmd;
        msg_send_receive(&m, &m, transceiver_pid);

        // cc110x_send() returns int8_t
        int8_t sent_len = (int8_t) m.content.value;
        if (sent_len <= 0)
            stats.radio_send_fail += 1;

        LED_GREEN_OFF;
    }
}

void *radio_to_uart_thread(void *arg)
{
    (void) arg;

    msg_t m;
    radio_packet_t *p;

    transceiver_register(TRANCSCEIVER, thread_getpid());

    while (1)
    {
        msg_receive(&m);

        LED_YELLOW_ON;

        if (m.type == PKT_PENDING)
        {
            p = (radio_packet_t *) m.content.ptr;
            posix_write(xport_pid, (char *) p->data, p->length);
            p->processing--;
        }
        else if (m.type == ENOBUFFER)
        {
            stats.radio_buffer_full += 1;
        }

        LED_YELLOW_OFF;
    }
}