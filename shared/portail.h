#ifndef PORTAIL_H__
#define PORTAIL_H__


#include "cc110x_legacy.h"

#define THREAD_STACKSIZE_DEFAULT (196)

#define GATE_PROXY_RADIO_ADDR   (1)
#define GATEWAY_RADIO_ADDR      (2)

// basically the narrowest piece of the data pipeline
#define PORTAIL_MAX_DATA_SIZE (CC1100_MAX_DATA_LENGTH)


#endif /* end of include guard: PORTAIL_H__ */