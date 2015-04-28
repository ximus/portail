#ifndef GATE_TELEMETRY_H
#define GATE_TELEMETRY_H

#include <stdint.h>
#include "stdbool.h"
#include "vtimer.h"
#include "timex.h"

typedef enum {
    GATE_STATE_UNKNOWN,
    GATE_STATE_OPENING,
    GATE_STATE_CLOSING,
    GATE_STATE_CLOSED,
    GATE_STATE_OPENED_FULLY,
    GATE_STATE_OPENED_PARTLY
} gate_state_t;

typedef struct {
    uint16_t ref_speed;     // units/s
    uint16_t distance;
    uint16_t velocity_sampling_hz;
    // Gate has a very reactive stop. When you hit the remote button, it stops
    // movement quickly. This measure is more of the time it takes for the gate
    // exit its current state (stop) and start moving, towards the next state.
    // would there be any problem representing this as its
    // native type of timex_t? thinking serialization?
    uint64_t reactivity_us;
} telemetry_t;

extern telemetry_t telemetry;


// T for Telemetry
#define T_MSG_LEARN_SPEED   (12345)
#define T_MSG_OK            (12346)


gate_state_t gate_get_state(void);
// int gate_get_movement(int8_t*);
int gate_get_position(uint16_t*);
int gate_get_position_relative(uint8_t*);
int gate_get_velocity(uint16_t*);
void telemetry_wakeup(void);
void telemetry_sleep(void);
void telemetry_learn_speed(void);
bool telemetry_needs_teaching(void);
int telemetry_learn_reactivity(void);

char *gate_state_to_s(gate_state_t);


#define GATE_STATUS_POLL_INTERVAL_MS (500)

static inline bool gate_is_moving(void)
{
    gate_state_t state = gate_get_state();
    return state == GATE_STATE_OPENING || state == GATE_STATE_CLOSING;
}

// TODO: timeout?
static inline void gate_wait_until_fully_opened(void)
{
    while (gate_get_state() != GATE_STATE_OPENED_FULLY)
    {
        vtimer_sleep(timex_set(0, GATE_STATUS_POLL_INTERVAL_MS));
    }
}

// TODO: timeout?
static inline void gate_wait_until_closed(void)
{
    while (gate_get_state() != GATE_STATE_CLOSED)
    {
        vtimer_sleep(timex_set(0, GATE_STATUS_POLL_INTERVAL_MS));
    }
}

#endif