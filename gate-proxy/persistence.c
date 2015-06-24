#include "stdint.h"
#include "flashrom.h"
#include "msp430.h"
#include "board.h"

#include "gate_telemetry.h"
#include "laser.h"

#define ENABLE_DEBUG    (0)
#include "debug.h"

// marker placed at begining of info segment upon persist to attest that
// an existing state is persisted
static const uint persistence_token = 0xAAAA;

#define PERSIST_ADDR  (INFOMEM)

typedef struct {
    void* ptr;
    uint  len;
} state_element_t;


/**
* @brief Definition of what gets persisted is here
*/
static const state_element_t state_elements[] = {
    { &telemetry, sizeof(telemetry) },
    // laser.value gets needlessly persisted, ... and restored? should be fine
    { &laser,     sizeof(laser) }
};


static const uint8_t element_count =
    sizeof(state_elements) / sizeof(state_elements[0]);

void persist_state(void)
{
    flashrom_erase((uint8_t *) PERSIST_ADDR);

    uint offset = PERSIST_ADDR;
    flashrom_write(
        (uint8_t*) offset,
        (uint8_t*) &persistence_token,
        sizeof(persistence_token)
    );
    offset += sizeof(persistence_token);

    for (int i = 0; i < element_count; ++i) {
        flashrom_write(
            (uint8_t*) offset,
            state_elements[i].ptr,
            state_elements[i].len
        );
        DEBUGF(
          "persisted %d bytes of state from 0x%X to 0x%X\n",
          state_elements[i].len,
          offset,
          state_elements[i].ptr
        );
        offset += state_elements[i].len;
    }
}

int restore_state(void)
{
    if (*((uint16_t *) PERSIST_ADDR) ==  persistence_token) {
        uint offset = PERSIST_ADDR + sizeof(persistence_token);
        for (int i = 0; i < element_count; ++i)
        {
            memcpy(
                state_elements[i].ptr,
                (uint*)offset,
                state_elements[i].len
            );
            DEBUGF(
              "restored %d bytes of state from 0x%X to 0x%X\n",
              state_elements[i].len,
              offset,
              state_elements[i].ptr
            );
            offset += sizeof(state_elements[i].len);
        }
        return 1;
    }
    else {
        persist_state();
        return 0;
    }
}