#include "shell_commands.h"
#include "posix_io.h"
#include "shell.h"
#include "board_uart0.h"
#include "board_uart0.h"
#include "errors.h"
#include "gate_telemetry.h"
#include "gate_control.h"
#include "gate_radio.h"
#include "net_monitor.h"
#include "laser.h"
#include "stdio.h"
#include "stdlib.h"
#include "thread.h"
#include "persistence.h"
#include "config.h"

static const int ESCAPE_KEY = 27;

static shell_t shell;

static void setup_command(int argc, char **argv);
static void learn_code_command(int argc, char **argv);
static void info_command(int argc, char **argv);
static void telemetry_on_command(int argc, char **argv);
static void telemetry_off_command(int argc, char **argv);
static void gate_open_command(int argc, char **argv);
static void gate_close_command(int argc, char **argv);
static void radio_test_command(int argc, char **argv);
#if LASER_STREAM
static void laser_stream_command(int argc, char **argv);
#endif

static const shell_command_t shell_commands[] = {
    { "setup", "calibrate gate", setup_command },
    { "learn_code", "gate radio code", learn_code_command },
    { "info", "display gate info", info_command },
    { "telemetry_on", "", telemetry_on_command },
    { "telemetry_off", "", telemetry_off_command },
    { "open", "", gate_open_command },
    { "close", "", gate_close_command },
    { "radio_test", "", radio_test_command },
    #if LASER_STREAM
    { "laser_stream", "read laser", laser_stream_command },
    #endif
    { NULL, NULL, NULL }
};

static int shell_readc(void);
// test me
uint8_t readline(char *buff, uint8_t len)
{
    int c;
    uint8_t count = 0;
    while (1)
    {
        c = uart0_readc();
        if (c == '\r' || c == '\n')
        {
            return count;
        }
        if (buff != NULL)
        {
            if (++count >= len)
                return count;
            *buff++ = c;
        }
    }
}

static inline void wait_for_enter_key(void)
{
    (void) readline(NULL, 0);
}

// #define MAX_CHARS_INPUT (6)
// static char input_buffer[MAX_CHARS_INPUT];

static void telemetry_on_command(int argc, char **argv)
{
    telemetry_wakeup();
}

static void telemetry_off_command(int argc, char **argv)
{
    telemetry_sleep();
}

static void learn_code_command(int argc, char **argv)
{
    // Learning gate code
    int tries = 0;
    while (gate_radio_learn_code() != 0) {
        if (++tries == LEARN_GATE_CODE_TRIES) {
            puts("failed");
            return;
        }
        puts("trying again");
    }
    puts("done");
}

static void setup_command(int argc, char **argv)
{
    puts("make sure no one will interact with gate and press [Enter]");
    wait_for_enter_key();

    // TODO: assert gate code has been learnt before continuing
    telemetry_wakeup();

    // must be learned first as upcoming gate close and open commands require it
    puts("learning reactivity (1/4)");
    telemetry_learn_reactivity();

    // puts("close gate and press [Enter]");
    // wait_for_enter_key();
    puts("learning closed limit (2/4)");
    gate_close();
    gate_wait_until_closed();
    laser_learn_closed_limit();

    // puts("open gate and press [Enter]");
    // wait_for_enter_key();
    puts("learning open limit (3/4)");
    gate_open();
    gate_wait_until_fully_opened();
    laser_learn_opened_limit();

    // uint16_t dist = 0;
    // do
    // {
    //     puts("measure, then input gate opening (mm) then press [Enter]");
    //     readline(input_buffer, MAX_CHARS_INPUT);
    //     errno = 0;
    //     dist = strtol(input_buffer, NULL, 10);
    // } while (errno || dist == 0);
    // telemetry.distance = dist;

    // puts("let gate close and press [Enter] once it starts moving");
    // wait_for_enter_key();

    puts("learning avg speed (4/4");
    gate_close();
    telemetry_learn_speed();

    puts("saving state...");
    persist_state();

    puts("Done");
}

static void info_command(int argc, char **argv)
{
    int err;

    printf("laser on: %s\n", laser.state == LASER_READING ? "yes":"no");

    if (telemetry_needs_teaching() || laser_needs_teaching())
    {
        puts("* setup need *\n");
    }
    printf("distance: %d mm\n", telemetry.distance);
    printf("opened value: %d\n", laser.opened_value);
    printf("closed value: %d\n", laser.closed_value);
    printf("resolution: %d\n", laser.resolution);
    printf("reference speed: %d /ms\n", telemetry.ref_speed);
    printf("gate state: %s\n", gate_state_to_s(gate_get_state()));

    uint16_t velocity;
    printf("velocity: ");
    err = gate_get_velocity(&velocity);
    if (err) {
        print_error(err);
    }
    else {
        printf("%d\n", velocity);
    }

    // int8_t movement;
    // printf("movement: ");
    // err = gate_get_movement(&movement);
    // if (err) {
    //     print_error(err);
    // }
    // else{
    //     printf("%d\n", movement);
    // }

    uint16_t position;
    printf("position: ");
    err = gate_get_position(&position);
    if (err) {
        print_error(err);
    }
    else {
        printf("%d\n", position);
    }

    printf("sample rate: %d hz\n", telemetry.velocity_sampling_hz);
}


static void gate_open_command(int argc, char **argv)
{
    int err = gate_open();

    if (err == 0) {
        puts("did nothing");
    }

    if (err < 0) {
        puts("error");
    }
}

static void gate_close_command(int argc, char **argv)
{
    int err = gate_open();

    if (err == 0) {
        puts("did nothing");
    }

    if (err < 0) {
        puts("error");
    }
}

static void radio_test_command(int argc, char **argv)
{
    netm_qual_test_start(true);
    printf("press [Enter] to exit\n\n");
    wait_for_enter_key();
    netm_qual_test_stop();
}

#if LASER_STREAM
#include "circular_buffer.h"
static void laser_stream_command(int argc, char **argv)
{
    // puts("press [c] to stop");
    laser_on();
    uint16_t length = strtol(argv[1], NULL, 10);
    if (errno) {
        printf("error: arg not an integer\n");
        return;
    }
    // while (1)
    for (int i = 0; i < length; ++i) {
        // blocking:
        // if (uart0_readc() == 'c')
        // {
        //     puts("stopped");
        //     return;
        // }
        if (!circ_buffer_is_empty(&laser_stream_cib))
        {
            int index = circ_buffer_get(&laser_stream_cib);
            printf("%d\n", laser_stream_buffer[index]);
        }
    }
}
#endif

void app_shell_run(void)
{
    board_uart0_init();
    (void) posix_open(uart0_handler_pid, 0);
    shell_init(&shell, shell_commands, UART0_BUFSIZE, uart0_readc, uart0_putc);
    shell_run(&shell);
}