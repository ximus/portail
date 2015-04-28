#include "app_shell.h"
#include "gate_telemetry.h"
#include "persistence.h"
#include "laser.h"
#include "net_api/endpoint.h"

// TODO: add radio transport security?
// TODO: program a button to learn gate code
// TOOD: gracefully degrade in abscence of laser, support code send

void main(void)
{
    vtimer_init();
    laser_init();
    uart_init();

    // restore any existing persisted state
    restore_state();

    api_endpoint_start();
    app_shell_run();
    // TODO: set led if teaching needed
}

// int main(void) {
//     transceiver_init(TRANSCEIVER_DEFAULT);
//     transceiver_start();

//     transceiver_register(TRANSCEIVER_DEFAULT, thread_getpid());

//     // init green led
//     P3OUT &= ~BIT6;
//     P3DIR |= BIT6;

//     while (1) {
//         msg_receive(&m);

//         switch(m.sender_pid) {
//             case transceiver_pid:
//                 // LED GREEN ON
//                 P3OUT |= BIT6;
//                 sleep(1);
//                 // LED GREEN OFF
//                 P3OUT &= ~BIT6;
//                 break;
//         }
//     }
// }