#include <kernel_types.h>

extern volatile kernel_pid_t relay_pid;

enum relay_msg_t {
  RELAY_PKT
};

int relay_start(void);