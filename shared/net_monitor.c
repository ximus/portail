#include "stdbool.h"
#include "string.h"
#include "thread.h"
#include "timex.h"
#include "led.h"
#include "vtimer.h"
#include "buttons.h"
#include "leds.h"
#include "net_layer.h"
#include "net_monitor.h"
#include "portail.h"

static kernel_pid_t thread_pid = KERNEL_PID_UNDEF;

char stack_buffer[THREAD_STACKSIZE_DEFAULT];

static bool print_to_shell = false;
static bool qual_test_is_running = false;

/* set to web standard ping port number */
#define PING_PORT (4)
static const uint8_t PING_BODY[] = "ping";
static const uint8_t PONG_BODY[] = "pong";
#define PING_BODY_SIZE (sizeof(PING_BODY)-1)
#define PONG_BODY_SIZE (sizeof(PONG_BODY)-1)
#define PING_TICK_MS    (250ul)
#define PING_ERR_NUM    (5)
#define PING_TIMEOUT_MS (PING_ERR_NUM * PING_TICK_MS)

#define _MSG_TYPE_BASE         (0x4822) // randomly chosen
#define PING_TICK_MSG_TYPE     (_MSG_TYPE_BASE + 1)
#define PING_TIMEOUT_MSG_TYPE  (_MSG_TYPE_BASE + 2)

static timex_t ping_tick_interval;
static timex_t ping_timeout_interval;
static vtimer_t ping_tick_timer;
static vtimer_t ping_timeout_timer;

#define QUAL4   (116)
#define QUAL3   (QUAL4-32) //84
#define QUAL2   (QUAL3-32) //52
#define QUAL1   (QUAL2-32) //20
#define LED_TIM_PERIOD (90) // ms
#define QUAL_PER(q,v)  (LED_TIM_PERIOD/3 + 10*(v-q))

static void update_quality_test(uint8_t rssi)
{
    /* signal quality bars (like on a cellphone) */
    uint8_t bars;

    if (rssi > QUAL3) {
        led_off(GREEN|YELLOW);
        led_blink(RED, QUAL_PER(QUAL3, rssi));
        bars = 1;
    }
    else if (rssi > QUAL2) {
        led_off(GREEN);
        led_blink(YELLOW, QUAL_PER(QUAL2, rssi));
        led_on(RED);
        bars = 2;
    }
    else if (rssi > QUAL1) {
        led_blink(GREEN, QUAL_PER(QUAL1, rssi));
        led_on(YELLOW|RED);
        bars = 3;
    }
    else {
        led_on(GREEN|YELLOW|RED);
        bars = 4;
    }

    if (print_to_shell)
    {
        /* TODO: add line delete char */
        printf("Signal: [");
        for (int i = 0; i < 4; ++i)
        {
            i <= bars ? printf("#") : printf(" ");
        }
        printf("] (RSSI=%d)", rssi);
    }
}

static bool timeout_timer_is_running = false;

static void clear_ping_timeout(void)
{
    vtimer_remove(&ping_timeout_timer);
    timeout_timer_is_running = false;
}

static void set_ping_tick(void)
{
    vtimer_set_msg(
        &ping_timeout_timer,
        ping_timeout_interval,
        thread_pid,
        PING_TIMEOUT_MSG_TYPE, NULL
    );
    timeout_timer_is_running = true;
}

static void set_ping_timeout(void)
{
    vtimer_set_msg(
        &ping_tick_timer,
        ping_tick_interval,
        thread_getpid(),
        PING_TICK_MSG_TYPE, NULL
    );
}

static void *thread(void *arg)
{
    int sock = netl_socket(SOCK_DATA);

    netl_bind(sock, PING_PORT);

    msg_t msg;

    while(1)
    {
        msg_receive(&msg);

        if (msg.type == NETL_RCV_MSG_TYPE)
        {
            static uint8_t buffer[sizeof(PING_BODY)];
            static frominfo_t info;

            netl_recv(sock, buffer, sizeof(buffer), &info);
            int msg_is_ping = !strncmp(buffer, PING_BODY, PING_BODY_SIZE);
            int msg_is_pong = !strncmp(buffer, PONG_BODY, PONG_BODY_SIZE);

            if (msg_is_ping)
            {
                /* send pong */
                netl_send(sock, PONG_BODY, PONG_BODY_SIZE);
            }

            if (msg_is_pong && qual_test_is_running)
            {
                clear_ping_timeout();
                update_quality_test(info.rssi);
            }
        }
        else if (msg.type == PING_TICK_MSG_TYPE)
        {
            if (qual_test_is_running)
            {
                /* send ping */
                netl_send(sock, PING_BODY, PING_BODY_SIZE);
                set_ping_tick();
                if (!timeout_timer_is_running)
                {
                    set_ping_timeout();
                }
            }
        }
        else if (msg.type == PING_TIMEOUT_MSG_TYPE)
        {
            timeout_timer_is_running = false;

            led_off(GREEN|YELLOW|RED);
            led_blink(GREEN|YELLOW|RED, LED_TIM_PERIOD);
        }
    }

    UNREACHABLE();
}

static void handle_button_press(void)
{

    if (qual_test_is_running)
    {
        netm_qual_test_stop();
    }
    else {
        netm_qual_test_start(false);
    }
}

void netm_qual_test_start(bool shell_print)
{
    if (!qual_test_is_running)
    {
        led_aquire();
        msg_t m;
        m.type = PING_TICK_MSG_TYPE;
        msg_send(&m, thread_pid);
        qual_test_is_running = true;
    }
    print_to_shell = shell_print;
}

void netm_qual_test_stop(void)
{
    if (qual_test_is_running)
    {
        led_release();
        qual_test_is_running = false;
    }
}

void netm_start(void)
{
    ping_tick_interval    = timex_from_uint64(PING_TICK_MS * MS_IN_USEC);
    ping_timeout_interval = timex_from_uint64(PING_TIMEOUT_MS * MS_IN_USEC);

    if (thread_pid == KERNEL_PID_UNDEF)
    {
        thread_pid = thread_create(
          stack_buffer,
          sizeof(stack_buffer),
          THREAD_PRIORITY_MAIN - 2,
          0,
          thread,
          NULL,
          "net_monitor"
        );
    }

    on_button2(&handle_button_press);
}