#include "relay.h"
#include "uart.h"
#include "transceiver.h"
#include "xport.h"

// TODO: code analysis: https://github.com/terryyin/lizard
// http://www.maultech.com/chrislott/resources/cmetrics/

// TODO: add message signing or encryption to requests
// TODO: i could use one of the buttons to put the gateway
// into echo mode to quickly run tests. It would be good
// practice at dynamically shutting down/creating threads.

int main(void)
{
    relay_start();

    return 0;
}