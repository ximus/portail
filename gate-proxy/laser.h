#ifndef LASER_H
#define LASER_H

#include "intrinsics.h"

typedef enum {
    LASER_OFF,
    LASER_STARTING,
    LASER_READING,
    LASER_LEARNING
} laser_state_t;

typedef struct {
    uint16_t ema_alpha;
    uint16_t opened_value;//:12;  // 12-bit adc max value 2^12 = 4096
    uint16_t closed_value;//:12;  // 12-bit adc max value 2^12 = 4096
    // not adc resolution, gate distance resolution
    // helps stabalize telemetry values by coarsing up the value range
    uint16_t resolution;

    volatile laser_state_t state;
    volatile uint16_t value;
} laser_t;

extern laser_t laser;

// const uint16_t LASER_MAX_VALUE = 4096;

void laser_init(void);
void laser_on(void);
void laser_off(void);
int laser_learn_closed_limit(void);
int laser_learn_opened_limit(void);
bool laser_needs_teaching(void);
uint8_t laser_value_to_relative(uint16_t);

#if LASER_STREAM
#warning "Laser stream included"
#include "circular_buffer.h"
#define LASER_STREAM_BUFSIZE (64)
extern uint8_t laser_stream_buffer[LASER_STREAM_BUFSIZE];
extern volatile circ_buffer_t laser_stream_cib;
#endif


// TODO: I feel i should timeout this waiting period in case there is a failure
// with the laser or laser link. This snippet can be used as inspiration:
// int tries = 6;
// uint timeout_ms = 500;
// timex_t timeout = timex_set(0, timeout_ms / tries); // 1/2 second
// while (gate_get_state() == GATE_STATE_UNKNOWN) {
//     thread_sleep(timeout);
//     if (--tries == 0) {
//         return -1;
//     }
// }
static inline void laser_wait_for_reading(void)
{
    while (laser.state == LASER_STARTING)
    {
        __no_operation();
    }
}

#endif