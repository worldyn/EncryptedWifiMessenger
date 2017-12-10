#include "spinny.h"
#include <pic32mx.h>
#include <stdint.h>

void spinner_setup() {
 /* PORTB.2 is analog pin with potentiometer*/
  AD1PCFG = ~(1 << 2);
  TRISBSET = (1 << 2);
  /* Use pin 2 for positive */
  AD1CHS = (0x2 << 16);

  /* Data format in uint32, 0 - 1024
  Manual sampling, auto conversion when sampling is done
  FORM = 0x4; SSRC = 0x7; CLRASAM = 0x0; ASAM = 0x0; */
  AD1CON1 = (0x4 << 8) | (0x7 << 5);

  AD1CON2 = 0x0;
  AD1CON3 |= (0x1 << 15);

  /* Turn on ADC */
  AD1CON1 |= (0x1 << 15); 
  /*  
  IFSCLR(1) = (1 << 1); // clear interrupt flag
  IECSET(1) = (1 << 1); // activae interrupt
  IPCSET(6) = 0xEF << 24; // set priority
  */ 
  // Set done to 0.
  AD1CON1 &= ~1;
  // Set sample to 1.
  AD1CON1 |= 2;
}
