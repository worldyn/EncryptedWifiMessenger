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

void labinit( void )
{
  volatile int* trise_point = (volatile int*) 0xbf886100;
  *trise_point &= ~0xff;
  
  TRISD |= 0x00000fd0; 
  
  // Assignment 2 initialize timer 2 for timeouts every 100 ms
  // Clock frequency is 80 MHz. 
  // I use 1:256 scaling and Period register is 3125. 
  //  80*10^6 / 256 / 31250 = 10 periods per second <-> one period is 1/10 sec = 100 ms 

  T2CON |= 0x0070; // use 1:256 prescale value and start timer
  TMR2 = 0x0; // Set timer value to 0   
  PR2 = 31250; // Period Register
  IFS(0) &= 0b1111111011111111; // set timer #2 interrupt flag (bit #8) to 0.
  T2CON |= 0x8000;
  
  IEC(0) |= 0x100; // activate interrupts for timer 2. 
  IPC(2) |= 0b11111; // set highest priority for timer 2.
  
  IEC(0) |= 0x800;
  IPC(2) |= 0x1f000000;

  asm volatile("ei");

  display_debug(0xbf881060);

  return;
}

/* This function is called repetitively from the main program */
void labwork( void )
{
  prime = nextprime(prime);
  display_string(0, itoaconv(prime));
  display_update();
}
