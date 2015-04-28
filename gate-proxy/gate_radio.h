#ifndef GATE_OPEN_H
#define GATE_OPEN_H

#include "stdint.h"
#include "gate_telemetry.h"

extern const uint8_t gate_toggle_code[5];

void gate_radio_send_code(void);
uint8_t gate_radio_strobe(gate_state_t target_state);
uint8_t gate_radio_learn_code(void);

#endif