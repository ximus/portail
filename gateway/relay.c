#include <thread.h>
#include <transceiver.h>
#include "xport.h"
#include "relay.h"
#include "posix_io.h"


#define RCV_BUFFER_SIZE       (64)

static char uart_to_radio_stack[THREAD_STACKSIZE_DEFAULT];
static char radio_to_uart_stack[THREAD_STACKSIZE_DEFAULT];

#define TRANCSCEIVER TRANSCEIVER_DEFAULT

volatile kernel_pid_t uart_to_radio_pid = KERNEL_PID_UNDEF;
volatile kernel_pid_t radio_to_uart_pid = KERNEL_PID_UNDEF;

static struct {
  uint radio_buffer_full;
} stats;

void *uart_to_radio_thread(void*);
void *radio_to_uart_thread(void*);


// expects a transceiver initialized and started
// expects xport initialized
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
      "uart_to_radio_thread"
    );

    radio_to_uart_pid = thread_create(
      radio_to_uart_stack,
      sizeof(radio_to_uart_stack),
      THREAD_PRIORITY_MAIN - 2,
      CREATE_STACKTEST,
      radio_to_uart_thread,
      NULL,
      "radio_to_uart_thread"
    );

    transceiver_register(TRANCSCEIVER, radio_to_uart_thread);
}

void *uart_to_radio_thread(void *arg)
{
    (void) arg;

    msg_t m;
    radio_packet_t radio_pkt;
    transceiver_command_t tcmd;
    static char buffer[PORTAIL_MAX_DATA_SIZE];

    radio_pkt.data = buffer;

    tcmd.transceivers = TRANCSCEIVER;
    tcmd.data = &radio_pkt;

    while (1)
    {
        radio_pkt.length = posix_read(xport_pid, buffer, sizeof(buffer));
        m.type = SND_PKT;
        m.content.ptr = (char *) &tcmd;
        msg_send_receive(&m, &m, transceiver_pid);

        int8_t sent_len = (int8_t) m.content.value;
        if (sent_len == 0)
        {
            stats.radio_sent_nothing += 1;
        }
        if (sent_len != radio_pkt.length)
        {
            /* code */
        }
    }
}

void *radio_to_uart_thread(void *arg)
{
    (void) arg;

    msg_t m;
    radio_packet_t *p;

    while (1)
    {
        msg_receive(&m);

        if (m->type == PKT_PENDING)
        {
            p = (radio_packet_t *) m.content.ptr;
            posix_write(xport_pid, p->data, p->length);
            p->processing--;
        }
        else if (m->type == ENOBUFFER)
        {
            stats.radio_buffer_full += 1;
        }
    }
}