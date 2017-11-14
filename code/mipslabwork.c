/* mipslabwork.c

   This file written 2015 by F Lundevall
   Updated 2017-04-21 by F Lundevall

   This file should be changed by YOU! So you must
   add comment(s) here with your name(s) and date(s):

   This file modified September 2017  by Adam Jacobs.

   For copyright and licensing, see file COPYING */

#include <stdint.h>   /* Declarations of uint_32 and the like */
#include <pic32mx.h>  /* Declarations of system-specific addresses etc */
#include "mipslab.h"  /* Declatations for these labs */

int prime = 1234567;

int mytime = 0x5957;

int timeoutcount = 0;

char textstring[] = "text, more text, and even more text!";

/* Interrupt Service Routine */
void user_isr( void )
{

  if((IFS(0) >> 11) & 0x1) { // if bit 11 is 1 ; bit 11 is interrupt flag for sw2 switch
    IFS(0) &= 0xfffff7ff;

    tick(&mytime);
    tick(&mytime);
    tick(&mytime);

    display_string(3, textstring);
    display_update();
  }

  if((IFS(0) >> 8) & 1) {
    IFS(0) &= 0xfffffeff;
    timeoutcount++;

    if(timeoutcount >= 10) {
      timeoutcount = 0;
      time2string(textstring, mytime);
      display_string(3, textstring);
      display_update();
      tick( &mytime );
    }
  }
}

/* Lab-specific initialization goes here */
int ledCount = 0;

/* Lab-specific initialization goes here */
void labinit( void )
{
  // The addresses to TRISE and PORTE according to the task.
  volatile unsigned* tris_e = (volatile unsigned*) 0xbf886100;

  // Set the least sig byte to zero (ie output), leave the rest as is.
  *tris_e &= 0xFFFF00;

  // Set bits 5-11 to 1 (ie input). BTNn4-2 and all 4 switches
  TRISD |= 0x00000FE0;

  // Set bit 1 to 1 (ie input). BTN1
  TRISFSET = 0b0010;

  // Init the timer to 0.
  TMR2 = 0;
  // Set bit 15 to one to enable the timer.
  // Also set all three TCKPS bits to one (bits 4-6).
  T2CON |= 0x8070;
  // Set the period;
  PR2 = 31250;
  //PR2 = 3125; // 10th of a sec
  // Set the 8th bit of IFS0 (T2IF) to 0, ie reset the timeout flag.
  IFS(0) &= 0xFFFFFEFF;

  // Enable 8th bit if IEC0 (T2IE).
  IEC(0) |= 0x00000100;
  // Enable all three T2IP (bits 2-4) and all two T2IS (bits 0-1)
  // for MAXIMUM prio and sub prio.
  IPC(2) |= 0x0000001F;

  // Enable bit 11 of IEC0 (INT2IE)
  IEC(0) |= 0x00000800;
  // ENable all three INT2IP (26-28)  and most sig bit of INT2IS (bit 25)
  IPC(2) |= 0x1E000000;



  // Init analog wheel thing:

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

  // Set AD1IF to 0.
  IFS(1) &= 2;
  // Enable bit 1 of IEC1 (AD1IE).
  IEC(1) |= 1 << 1;
  // Enable all three AD1IP (bits 26-28) and all two AD1IS (bits 24-25)
  // for MAXIMUM prio and sub prio.
  //IPC(6) |= 0x1F000000;
  IPC(6) |= 0x0F000000;

  // Set done to 0.
  AD1CON1 &= ~1;
  // Set sample to 1.
  AD1CON1 |= 2;


  // Enable interrupts globally.
  enable_interrupt();
}

/* This function is called repetitively from the main program */
void labwork( void )
{
  prime = nextprime(prime);
  display_string(0, itoaconv(prime));
  display_update();
}
