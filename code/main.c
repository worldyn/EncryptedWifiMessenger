#include <stdint.h>   /* Declarations of uint_32 and the like */
#include <pic32mx.h>  /* Declarations of system-specific addresses etc */

int main(void) {

  /* Set up input buttons */

  /* Set up SPI as master */
  SPI2CON = 0;
  SPI2BRG = 4;
  /* SPI2STAT bit SPIROV = 0; */
  SPI2STATCLR = 0x40;
  /* SPI2CON bit CKP = 1; */
        SPI2CONSET = 0x40;
  /* SPI2CON bit MSTEN = 1; */
  SPI2CONSET = 0x20;
  /* SPI2CON bit ON = 1; */
  SPI2CONSET = 0x8000;

  display_init();
  display_string(0, "ENTER MESSAGE:");
  display_string(1, "input-text");
  display_string(2, "ANSWER:");
  display_string(3, "output-text");
  textbuffer[3][15] = 'A';
  display_update();

  inputinit();

  while (1) {
    work();
  }
  return 0;
}

void inputinit(void) {
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

	return 0;
}

void work(void) {
  delay(50);

  /* Start sampling, wait until conversion is done */
  AD1CON1 |= (0x1 << 1);
  while(!(AD1CON1 & (0x1 << 1)));
  while(!(AD1CON1 & 0x1));

  /* Get the analog value */
  unsigned int rot = ADC1BUF0;
  unsigned int offset = (rot-39)/39;
  char c = 'a';
  if (offset <= 25) {
    c += offset;
  }
  textbuffer[3][15] = c;
  display_debug(&rot);

  display_update();
}
