#include <stdio.h>
#include <msp430.h>

#include "relay.h"
// #include "transceiver.h"
#include "xport.h"

// TODO: code analysis: https://github.com/terryyin/lizard
// http://www.maultech.com/chrislott/resources/cmetrics/

int main(void)
{
  // init green led
  P3OUT &= ~BIT6;
  P3DIR |= BIT6;
  // init red led
  P1OUT &= ~BIT7;
  P1DIR |= BIT7;
  // init yellow led
  P3OUT &= ~BIT7;
  P3DIR |= BIT7;

  xport_init();
  // transceiver_init(TRANSCEIVER_DEFAULT);
  // transceiver_start();

  relay_start();

  return 0;
}