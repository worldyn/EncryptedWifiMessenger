/*
 *  messaging main file
 */
#include <pic32mx.h>
#include <stdint.h>
#include <stdbool.h>
#include "i2c.h"
#include "settings.h"
#include "communication.h"
#include "spinny.h"

// character buffer for messages
char c = 'a';
// message buffer
char msg[17] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0};
// keep track of what column message is on
uint8_t column = 0;

#define WRITE_ROW 1
#define READ_ROW 3

void write_char(char cc) {
  textbuffer[WRITE_ROW][column] = cc;
  msg[column] = cc;
  if (column < 15) {
    ++column;
    display_string(WRITE_ROW, msg);
    display_update();
  }
}

void backspace() {
  if (column > 0) {
    textbuffer[WRITE_ROW][column] = 0;
    msg[column] = 0;
    --column;
    c = msg[column-1];
    
  }
}

const uint8_t non[8] = {1,2,3,4,5,6,7,8};
void send_message() {
  send_enc(sizeof(msg), msg);
}

void recieve_message() {
  show_message(READ_ROW, recv_dec_message());
}


/* Returns int where all bits are 0 except
 * bit 0: status of BTN1
 * bit 2: status of BTN2
 * bit 3: status of BTN3
 * bit 4: status of BTN4
 */
int getbtns()
{
  int d = (PORTD >> 4) & 0x0000000E;
  int f = (PORTF >> 1) & 0x00000001;
  return d | f;
}

void wheel_update() {
  /* Start sampling, wait until conversion is done */
  AD1CON1 |= (0x1 << 1);
  while(!(AD1CON1 & (0x1 << 1)));
  while(!(AD1CON1 & 0x1));
	
	unsigned int val = ADC1BUF0;

  /* Get the analog value */
  unsigned int offset = (val-39)/39;
  // write correct value to global c variable
  c = 'a';
  if (offset <= 25) {
    c += offset;
  }
  textbuffer[WRITE_ROW][column] = c;
  display_update();
}

void check_btns() {
	uint8_t btns = getbtns();
  if (btns & (1 << 0)) write_char(c);
  if (btns & (1 << 1)) write_char(' ');
  if (btns & (1 << 2)) backspace();
  if (btns & (1 << 3)) send_message();
}

int main(void) {
	uint16_t temp;
	char buf[32], *s, *t;

	/* Set up peripheral bus clock */
	OSCCON &= ~0x180000;
	OSCCON |= 0x080000;
	
	/* Set up output pins */
	AD1PCFG = 0xFFFF;
	ODCE = 0x0;
	TRISECLR = 0xFF;
	PORTE = 0;
	
  /* Timer and interrupt setup */
  // Timer 2 have 10 periods per second
  T2CON = 0; // Reset timer 2
  T2CON |= 0x70; // Prescaling 1:256
  TMR2 = 0; // set timer to zero
  PR2 = 31250;//31250; // period register
  IFS(0) &= 0xfeff; // timer2 interrupt flag = 0
  //IEC(0) |= 0x100; // activate interrupt for timer 2
  //IPC(2) |= 0x1f; // set highest sub priority for timer 2
  //IPC(2) |= 0x1f000000; // highest priority for timer 2
  //IEC(0) |= 0x800;
  T2CON |= 0x8000; // turn ON timer 2
  

	/* Output pins for display signals */
	PORTF = 0xFFFF;
	PORTG = (1 << 9);
	ODCF = 0x0;
	ODCG = 0x0;
	TRISFCLR = 0x70;
	TRISGCLR = 0x200;
	
  uart_setup();
  

	/* Set up input pins */
	TRISDSET |= 0xFE0;
	TRISFSET = (1 << 1);

	
	/* Set up SPI as master */
	SPI2CON = 0;
	SPI2BRG = 4;
	
	/* Clear SPIROV*/
	SPI2STATCLR &= ~0x40;
	/* Set CKP = 1, MSTEN = 1; */
  SPI2CON |= 0x60;
	
	/* Turn on SPI */
	SPI2CONSET = 0x8000;
	
	/* Set up input pins */
	TRISDSET = (1 << 8);
	TRISFSET = (1 << 1);
	
  // setup potentiometer
  spinner_setup();

  // initialize screen
	display_init();
  display_string(0, "");
  display_string(1, "");
  display_string(2, "");
  display_string(3, "");
  display_update();

  display_string(0, "Generating");
  display_string(1, "key and nonce...");
  display_update();
  gen_key_and_nonce();

  display_string(0,"Waiting for ");
  display_string(1, "connection...");
  display_update();
  while(!value_in_readbuf());
  initiate_contact();

  display_string(0, "Sending key");
  display_string(1, "and nonce...");
  display_update();
  send_key_and_nonce();
  
  display_string(0, "Recieving nonce");
  display_string(1, "and public key...");
  display_string(2,"");
  display_update();
  while(!value_in_readbuf());
  recv_nonce_and_key();

  display_string(0, "Calc. shared");
  display_string(1, "secret and keys...");
  display_update();
  gen_shared();
  gen_keys(); 

  display_string(0, "Generating");
  display_string(1, "nonce for enc...");
  display_update();
  gen_nonce();
  
  // Home screen
	display_string(0, "Enter message:");
  display_string(1, "");
	display_string(2, "Recieved:");
  display_string(3, "");
	display_update();
	
  textbuffer[WRITE_ROW][column] = 'a';
  display_update();

  // enable interrrupts
  //asm ("ei");
  
  /*
  * THE PROGRAM:
  */

  for (;;) {
    if (value_in_readbuf()) {
      recieve_message();
      continue;
    }
    // do something useful FOR once ;)
    wheel_update();
    if ((IFS(0) >> 8) & 1)  {
      IFSCLR(0) = (1 << 8); 
      check_btns();
    }
  }

	return 0;
}

// is ran everytime a message is intercepted
void user_isr(void) {
  /*if ((IFS(0) >> 27) & 1) { // check uart recieve interrupt
    IFSCLR(0) = (1 << 27); // clear interrupt flag  
   // show_envelope(2,recv_envelope());  
    PORTE = 3;
  }*/
  /*if ((IFS(1) >> 1) & 1) { // analog spinner interrupt flag 
    // do stuff
    wheel_update(ADC1BUF0); // send current value 
    IFSCLR(1) = (1 << 1);  // reset flag
    AD1CON1 &= ~1; // clr done
    AD1CON1 |= 2; // set sample bit
  }*/
}

