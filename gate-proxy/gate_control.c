#include "gate_radio.h"
#include "gate_telemetry.h"


// returns 1 on something to do and did something
// returns 0 on nothing to do and did nothing
// returns -1 error could not do anything
// returns -2 error on something to do but gate did not seem to budge
int gate_open(void)
{
    gate_state_t state = gate_get_state();

    if (state == GATE_STATE_UNKNOWN) {
        return -1;
    }

    if (state == GATE_STATE_OPENED_FULLY || state == GATE_STATE_OPENING) {
        // do nothing
        return 0;
    } else {
        if (gate_radio_strobe(GATE_STATE_OPENING) < 0) {
            return -1;
        }
        return 1;
    }
}

// returns 1 on something to do and did something
// returns 0 on nothing to do and did nothing
// returns -1 error could not do anything
// returns -2 error on something to do but gate did not seem to budge
int gate_close(void)
{
    gate_state_t state = gate_get_state();

    if (state == GATE_STATE_UNKNOWN) {
        return -1;
    }

    if (state == GATE_STATE_CLOSED || state == GATE_STATE_CLOSING) {
        // do nothing
        return 0;
    } else {
        if (gate_radio_strobe(GATE_STATE_CLOSING) < 0) {
            return -1;
        }
        return 1;
    }
}