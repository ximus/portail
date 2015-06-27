#include "msp430.h"
#include "board.h"
#include "stats.h"
#include "stdbool.h"

#include "laser.h"

// The duration of each pulse (corresponding to a push button “click”),
// and the period between multiple pulses, are defined as “T”: 0.04 seconds ≤ T ≤ 0.8 seconds.
#define LASER_PERIOD_TICKS  ((unsigned long) 5*F_CPU/100) /* F_CPU * 5% */

// Selected Response Speed
// Laser Enable Time
// Slow
// 150 ms
// Medium
// 60 ms
// Fast
// 51 ms

laser_t laser = {
    STATS_EMA_ALPHA(0.6),     // moving average alpha
    0,                        // opened value
    0,                        // closed value
    100,                      // resolution
    LASER_OFF,                // state
    0                         // value
};

#if LASER_STREAM
uint8_t laser_stream_buffer[LASER_STREAM_BUFSIZE];
volatile circ_buffer_t laser_stream_cib = CIRC_BUFFER_INIT(LASER_STREAM_BUFSIZE);
// cib_t laser_stream_cib;
#endif


// --------------------------------------------
// Laser control
// --------------------------------------------
static void adc_init(void);
static void precompute_values(void);

void laser_init(void)
{
    // init laser port (1.0)
    P1DIR |= BIT0;
    P1OUT &= ~BIT0;
    // init teach port (1.1)
    P1DIR |= BIT1;
    P1OUT &= ~BIT1;

    precompute_values();
    adc_init();
}

void laser_on(void)
{
    // turn on laser
    // should wait for laser to be ready ~150ms i think?
    P1OUT |= BIT0;

    laser.state = LASER_STARTING;

    // Enable ADC
    ADC12IE   |= ADC12IE0;            // enable ADC interrupt
    ADC12CTL0 |= ADC12ENC;            // Enable conversions
    ADC12CTL0 |= ADC12SC;             // start conversion

    #if LASER_STREAM
    circ_buffer_init(&laser_stream_cib, LASER_STREAM_BUFSIZE);
    #endif
}

void laser_off(void)
{
    // turn off adc
    ADC12IE   &= ~ADC12IE0;            // disable ADC interrupt
    ADC12CTL0 &= ~ADC12ENC;            // disable conversions
    ADC12CTL0 &= ~ADC12SC;             // stop conversion

    // turn off laser
    P1OUT &= ~BIT0;

    laser.state = LASER_OFF;
}

// --------------------------------------------
// Learning
// --------------------------------------------
bool laser_needs_teaching(void)
{
    return laser.opened_value <= laser.closed_value;
}

void goto_learning(void)
{
    if (laser.state == LASER_OFF)
    {
        laser_on();
        laser_wait_for_reading();
    }
    laser.state = LASER_LEARNING;
}

static bool value_changed = false;
static void wait_for_value_change(void)
{
    uint16_t value_changed = false;
    while (!value_changed) {
        __no_operation();
    }
}

// blocks while laser starts if off
int laser_learn_closed_limit(void)
{
    goto_learning();
    // triple pulse teach line
    for (uint8_t i = 0; i < 3; ++i)
    {
        P1OUT |= BIT1;
        __delay_cycles(LASER_PERIOD_TICKS);
        P1OUT &= ~BIT1;
        __delay_cycles(LASER_PERIOD_TICKS);
    }

    // what i'm looking for is to wait for the laser to change it's output
    // in response to the teaching, however I'm not sure this does it:
    wait_for_value_change();
    laser.closed_value = laser.value;

    return 0;
}

int laser_learn_opened_limit(void)
{
    if (laser.state != LASER_LEARNING)
    {
        return -1;
    }
    // single pulse
    P1OUT |= BIT1;
    __delay_cycles(LASER_PERIOD_TICKS);
    P1OUT &= ~BIT1;

    // what i'm looking for is to wait for the laser to change it's output
    // in response to the teaching, however I'm not sure this does it:
    wait_for_value_change();
    laser.opened_value = laser.value;

    laser.state = LASER_READING;
    return 0;
}

// --------------------------------------------
// Misc
// --------------------------------------------
// minimize math done on each adc tick by precomputing:
uint16_t position_range;
uint16_t slot_size;
uint16_t half_slot;
static void precompute_values(void)
{
    position_range = laser.opened_value - laser.closed_value;
    slot_size = position_range / laser.resolution;
    half_slot = slot_size / 2;
}

// returns percent value
uint8_t laser_value_to_relative(uint16_t value)
{
    return (value * 100) / position_range;
}

// --------------------------------------------
// ADC and Interrupt handling
// --------------------------------------------
static void adc_init(void)
{
    P2SEL |= BIT3;                 // ADC input pin P2.3
    ADC12CTL0 &= ~ADC12ENC;        // Disable ADC

    ADC12CTL0 = ADC12SHT0_12    // 1024 sample hold clock ticks
              + ADC12MSC        // multiple single conversions
              + ADC12ON;        // ADC On
    ADC12CTL1 = ADC12SHP           // pulse mode
              + ADC12CONSEQ_2            // Repeat-single-channel
              + ADC12SSEL1               // ACLK clock
              + ADC12DIV_7;              // clock / 8
    ADC12CTL2 = ADC12PDIV           // further clock / 4
              + ADC12TCOFF;         // temp sensor off
    ADC12MCTL0 = ADC12INCH_3;            // channel 3
}

// When the ADC12_A is not actively converting, the core is automatically
// disabled, and it is automatically reenabled when needed. The MODOSC is also
// automatically enabled when needed and disabled when not needed.

// idea: if gate is stopped near 1-2% of current min/max value,
// reset that value to that

// exponential moving average
// no glitch detection
// tiny = 1- 1/granularity
// a(i+1) = tiny*data(i+1) + (1.0-tiny)*a(i)

// core logic from here could be exported to gate_telemetry.c
interrupt(ADC12_VECTOR) __attribute__((naked)) adc_handler(void)
// interrupt(ADC12_VECTOR) adc_handler(void)
{
    __enter_isr();

    uint16_t reading = ADC12MEM0;

    if (laser.state == LASER_STARTING)
    {
        laser.state = LASER_READING;
    }
    else
    {
        reading = stats_ema_ui16(reading, laser.value, laser.ema_alpha);
    }


    // + half slot for rounding
    reading = reading + (half_slot);
    reading = reading - (reading % slot_size);

    #if LASER_STREAM
    int next_index = circ_buffer_add(&laser_stream_cib);
    // int next_index = cib_put(laser_stream_cib);
    // if (next_index == -1)
    // {
    //     cib_init(laser_stream_cib, LASER_STREAM_BUFSIZE);
    //     next_index = cib_put(laser_stream_cib);
    // }
    laser_stream_buffer[next_index] = reading;
    #endif

    laser.value = reading;
    value_changed = true;

    __exit_isr();
}