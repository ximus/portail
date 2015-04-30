#include "cc110x_legacy.h"

/**
 * While writing this module's api, I was quite new to writing c
 * and embeded software. I think it could use maturation, maybe
 * implement RIOT's posix api, but I walked a thin line of trying
 * to balance efficiency (like avoid memcpy) and readablity/complexity.
 * I'm sure significant refactoring could be done, beyond the fact that
 * it this whole app probably doesn't mandate riot and should be
 * bare metal.
 */

extern volatile kernel_pid_t xport_pid;

void xport_init(void);
void xport_register(kernel_pid_t);