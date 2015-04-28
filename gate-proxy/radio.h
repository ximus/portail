#ifndef RADIO_H
#define RADIO_H

#include "mutex.h"
#include "cc110x_legacy.h"

extern mutex_t radio_mutex;

static inline void radio_claim(void)
{
    // wait for any packet to send
    while (radio_state == RADIO_SEND_BURST) {
        nop();
    }
    mutex_lock(&radio_mutex);
}

static inline void radio_release(void)
{
    mutex_unlock(&radio_mutex);
}

#endif