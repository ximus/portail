// #include <stdio.h>
// #include "msp430.h"

// #include "shell.h"
// #include "shell_commands.h"
// #include "board_uart0.h"
// #include "posix_io.h"
// #include "cpu.h"

// // add low pass filter to banner input

// // The duration of each pulse (corresponding to a push button “click”),
// // and the period between multiple pulses, are defined as “T”: 0.04 seconds ≤ T ≤ 0.8 seconds.
// static void learn_gate_command(int argc, char **argv) {
//   // ask to position  first limit
//   // Triple-pulse the remote line
//   // ask to position second limit
//   // Single-pulse the remote line
//   // ask for
//   // check if gate moving, if not put in motion
//   // measure gate speed
// }

// uint32_t reading = 0;
// uint16_t count = 0;
// uint16_t isrcount = 0;

// static void read_gate_command(int argc, char **argv) {
//   puts("Setuping up ADC");
//   P2SEL |= BIT4;          // ADC input pin P2.4
//   ADC12CTL0 &= ~ADC12ENC;        // Disable ADC

//   ADC12CTL0 = ADC12SHT0_8 + ADC12ON; // 16 clock ticks, ADC On
//   ADC12CTL1 = ADC12SHP; // SMCLK
//   ADC12MCTL0 = ADC12INCH_4; // channel 4

//   __delay_cycles(75);

//   ADC12IE |= ADC12IE0; // enable ADC interrupt
//   ADC12CTL0 |= ADC12ENC;                    // Enable conversions

//   __bis_SR_register(GIE);

//   count = 0;
//   while (reading == 0) {
//       count += 1;
//       ADC12CTL0 |= ADC12SC;                     // Start conversion - software trigger
//       __delay_cycles(75);
//   }
//   printf("Gate voltage: %d\n", reading);
//   uint32_t distance_in_cm = (reading*490)/4095;
//   printf("Distance: %dcm\n", distance_in_cm);
//   printf(
//     "Distance: %d.%dm\n",
//     (uint32_t) (distance_in_cm / (uint32_t) 100),
//     (uint32_t) (distance_in_cm % (uint32_t) 100)
//   );
//   puts("reading complete");
//   reading = 0;
// }

// static void read_temp_command(int argc, char **argv) {

// }

// static const shell_command_t shell_commands[] = {
//     { "learn_gate", "calibrates gate laser", learn_gate_command },
//     { "read_gate", "read gate laser", read_gate_command },
//     { "read_temp", "read internal temperature", read_temp_command },
//     { NULL, NULL, NULL }
// };

// int main(void)
// {
//     puts("Enter Main");
//     WDTCTL = WDTPW + WDTHOLD; // Stop WDT

//     __bis_SR_register(GIE); // interrupts enabled


//     uart_init();

//     puts("Running shell");
//     posix_open(uart0_handler_pid, 0);
//     shell_t shell;
//     shell_init(&shell, shell_commands, UART0_BUFSIZE, uart0_readc, uart0_putc);
//     shell_run(&shell);

//     puts("End out");
//     return 0;
// }


// interrupt(ADC12_VECTOR) adc12_isr(void)
// {
//   // __enter_isr();
//   isrcount += 1;
//   reading = ADC12MEM0;      // Saves measured value
//   // __exit_isr();
// }