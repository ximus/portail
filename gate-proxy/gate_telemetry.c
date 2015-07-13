#include "gate_telemetry.h"
#include "laser.h"
#include "vtimer.h"
#include "hwtimer.h"
#include "timex.h"
#include "thread.h"
#include "priorities.h"
#include "stdlib.h"
#include "errors.h"
#include "gate_radio.h"
#include "portail.h"

#define ENABLE_DEBUG    (1)
#include "debug.h"


#define VELOCITY_STACKSIZE  (THREAD_STACKSIZE_DEFAULT)
#define TELEMETRY_STACKSIZE (THREAD_STACKSIZE_DEFAULT)

static char velocity_stack_buffer[VELOCITY_STACKSIZE];
static char stack_buffer[TELEMETRY_STACKSIZE];
kernel_pid_t telemetry_pid = KERNEL_PID_UNDEF;
kernel_pid_t velocity_pid  = KERNEL_PID_UNDEF;

// TODO: persist some of this
telemetry_t telemetry = {
    0,   // ref_speed
    0,   // distance
    10,  // velocity_sampling_hz
    (1 * 10^6)
};


static bool learning_speed = false;
static msg_t waiting_learn_msg;

// delta laser/µs
static int16_t velocity = 0;
static bool velocity_is_valid = false;

vtimer_t timer;
timex_t interval;

// TODO: try with pre-loading position in this thread.
void *velocity_thread(void *arg)
{
    (void) arg;

    int16_t       prev_laser_value;
    unsigned long prev_laser_value_ticks = 0;

    while (1)
    {
        if (laser.state == LASER_READING)
        {
            uint16_t value = laser.value;
            unsigned long ticks = hwtimer_now();


            // has prev_laser_value been set yet?
            if (prev_laser_value_ticks > 0)
            {
                uint32_t numticks = ticks - prev_laser_value_ticks;
                uint32_t elapsed_us = HWTIMER_TICKS_TO_US(numticks);
                velocity = (prev_laser_value - value) / elapsed_us;

                printf("ticks: %lu e_us: %lu\n", numticks, elapsed_us);
                // Learn speed if requested or speed not yet learned
                if (learning_speed)
                {
                    telemetry.ref_speed = abs(velocity);
                    learning_speed = false;
                    msg_reply(&waiting_learn_msg, &waiting_learn_msg);
                }
            }
            prev_laser_value = value;
            prev_laser_value_ticks = ticks;

            velocity_is_valid = true;

            interval = timex_set(0, 1000000 / telemetry.velocity_sampling_hz);
            timex_normalize(&interval);
            vtimer_set_wakeup(&timer, interval, thread_getpid());
        }
        else
        {
            velocity_is_valid = false;
            prev_laser_value_ticks = 0;
        }

        thread_sleep();
    }
}

void *telemetry_thread(void *arg)
{
    (void) arg;

    msg_t msg;

    while (1)
    {
        msg_receive(&msg);

        switch (msg.type) {
            case T_MSG_LEARN_SPEED:
                telemetry_wakeup();
                waiting_learn_msg = msg;
                learning_speed = true;
                break;
        }
    }
}

static void init(void)
{
    telemetry_pid = thread_create(
        stack_buffer,
        TELEMETRY_STACKSIZE,
        TELEMETRY_PRIORITY,
        CREATE_STACKTEST,
        telemetry_thread,
        NULL,
        "telemetry"
    );
    velocity_pid = thread_create(
        velocity_stack_buffer,
        VELOCITY_STACKSIZE,
        VELOCITY_PRIORITY,
        CREATE_STACKTEST,
        velocity_thread,
        NULL,
        "velocity"
    );
}

void telemetry_wakeup(void)
{
    if (telemetry_pid == KERNEL_PID_UNDEF) {
        init();
    }
    laser_on();
    laser_wait_for_reading();
    // TODO: needs interrupt disable?
    vtimer_remove(&timer);
    thread_wakeup(velocity_pid);
}

void telemetry_sleep(void)
{
    laser_off();
}

void telemetry_learn_speed(void)
{
    msg_t m;
    m.type = T_MSG_LEARN_SPEED;
    msg_send_receive(&m, &m, telemetry_pid);
}

int telemetry_learn_reactivity(void)
{
    gate_state_t init_state = gate_get_state();
    timex_t started_at = {0,0};
    timex_t now = {0,0};
    timex_t elapsed = {0,0};
    int tries = 3;

    while (1) {
        vtimer_now(&now);
        elapsed = timex_sub(now, started_at);

        // has timeout overrun? or is this the first run?
        if (timex_cmp(timex_set(3,0), elapsed) == 1)
        {
            // any tries left?
            if (--tries) {  // yes
                gate_radio_send_code();
                vtimer_now(&started_at);
            }
            else {  // no
                return 1;
            }
        }
        else
        {
            gate_state_t cur_state = gate_get_state();
            if (gate_is_moving() && cur_state != init_state) {
                // Jackpot :)
                break;
            }
        }
    }
    telemetry.reactivity_us = timex_uint64(elapsed);

    // put gate back in previous state
    gate_radio_strobe(init_state);

    return 0;
}

bool telemetry_needs_teaching(void)
{
    // return telemetry.ref_speed <= 0 || telemetry.distance <= 0;
    return telemetry.ref_speed <= 0;
}

// returns percent value
int gate_get_position_relative(uint8_t *dest)
{
    if (telemetry_needs_teaching() || laser_needs_teaching()) {
        return -E_CALIB_INVAL;
    }
    if (laser.state != LASER_READING) {
        return -E_LASER_NOT_RDY;
    }

    *dest = laser_value_to_relative(laser.value);

    return 0;
}

int gate_get_position(uint16_t *val)
{
    if (telemetry_needs_teaching() || laser_needs_teaching()) {
        return -E_CALIB_INVAL;
    }
    if (laser.state != LASER_READING) {
        return -E_LASER_NOT_RDY;
    }

    *val = laser.value;

    return 0;
}

int gate_get_velocity(uint16_t *val)
{
    if (telemetry_needs_teaching() || laser_needs_teaching()) {
        return -E_CALIB_INVAL;
    }
    if (laser.state != LASER_READING) {
        return -E_LASER_NOT_RDY;
    }

    *val = velocity;

    return 0;
}

gate_state_t gate_get_state(void)
{
    if (velocity_is_valid)
    {
        if (velocity > 0)
        {
            return GATE_STATE_OPENING;
        }
        if (velocity < 0)
        {
            return GATE_STATE_CLOSING;
        }
    }

    uint16_t position;
    int err = gate_get_position(&position);
    if (!err) {
        if (position == laser.closed_value)
        {
            return GATE_STATE_CLOSED;
        }
        if (position == laser.opened_value)
        {
            return GATE_STATE_OPENED_FULLY;
        }
        return GATE_STATE_OPENED_PARTLY;
    }
    return GATE_STATE_UNKNOWN;
}

char *gate_state_to_s(gate_state_t state)
{
    switch(state) {
        case GATE_STATE_UNKNOWN:
            return "UNKNOWN";
        case GATE_STATE_OPENING:
            return "OPENING";
        case GATE_STATE_CLOSING:
            return "CLOSING";
        case GATE_STATE_CLOSED:
            return "CLOSED";
        case GATE_STATE_OPENED_FULLY:
            return "OPEN_FULLY";
        case GATE_STATE_OPENED_PARTLY:
            return "OPEN_PARTLY";
    }
}

// char *gate_state_get_state_s(void)
// {
//     return gate_state_to_s(gate_get_state());
// }


// OPTIMIZE: could do away with this and use a "best guess" wait time instead
int gate_wait_until_position(uint8_t target_percent)
{
    uint16_t velocity;
    uint8_t  current_percent;
    do
    {
        gate_get_velocity(&velocity);
        gate_get_position_relative(&current_percent);
        if (velocity > 0) {
            if (target_percent > 0)
            {
                /* code */
            }
        }
        vtimer_sleep(timex_set(0, GATE_STATUS_POLL_INTERVAL_MS));
    }
    while (gate_get_state() != GATE_STATE_CLOSED);
}