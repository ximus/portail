#include <string.h>

#include "thread.h"
#include "posix_io.h"
#include "net_layer.h"

#include "portail.h"
#include "xport.h"
#include "relay.h"
#include "leds.h"
#include "hwtimer.h"


#define ENABLE_DEBUG    (1)
#include "debug.h"

static char uart_to_radio_stack[THREAD_STACKSIZE_DEFAULT];
static char radio_to_uart_stack[THREAD_STACKSIZE_DEFAULT];

kernel_pid_t uart_to_radio_pid = KERNEL_PID_UNDEF;
kernel_pid_t radio_to_uart_pid = KERNEL_PID_UNDEF;

static int sock = -1;


void *uart_to_radio_thread(void*);
void *radio_to_uart_thread(void*);

int relay_start(void)
{
    netl_init();
    xport_init();

    sock = netl_socket(SOCK_RAW);

    if (sock == -1) {
        return -1;
    }

    if (uart_to_radio_pid == KERNEL_PID_UNDEF)
    {
        uart_to_radio_pid = thread_create(
          uart_to_radio_stack,
          sizeof(uart_to_radio_stack),
          THREAD_PRIORITY_MAIN - 2,
          CREATE_STACKTEST,
          uart_to_radio_thread,
          NULL,
          "uart_to_radio"
        );
    }

    if (radio_to_uart_pid == KERNEL_PID_UNDEF)
    {
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

    return 0;
}

void *uart_to_radio_thread(void *arg)
{
    (void) arg;

    static uint8_t buffer[PORTAIL_MAX_DATA_SIZE];

    while (1)
    {
        DEBUG("uart_to_radio: waiting ...\n");

        int8_t len = posix_read(xport_pid, (char *) buffer, sizeof(buffer));

        DEBUG("uart_to_radio: relaying %d bytes\n", len);
        LED_GREEN_ON;

        netl_send(sock, buffer, sizeof(buffer));

        LED_GREEN_OFF;
    }
}

void *radio_to_uart_thread(void *arg)
{
    (void) arg;

    static uint8_t buffer[PORTAIL_MAX_DATA_SIZE];

    while (1)
    {
        int len = netl_recv(sock, buffer, sizeof(buffer), NULL);

        if (0 < len) {
          LED_YELLOW_ON;

          posix_write(xport_pid, (char *) buffer, len);

          LED_YELLOW_OFF;
        }
    }
}