#include <pic32mx.h>
#include <stdint.h>
#include "communication.h"
#include "settings.h"
#include "crash.h"
#include "chacha20.h"
#include "dh.h"
#include <stdbool.h>

/*
* uart stuff
*/

#define HEADER_LEN 9

int calculate_baudrate_divider(int sysclk, int baudrate, int highspeed) {
  int pbclk, uxbrg, divmult;
  unsigned int pbdiv;
  
  divmult = (highspeed) ? 4 : 16;
  /* Periphial Bus Clock is divided by PBDIV in OSCCON */
  pbdiv = (OSCCON & 0x180000) >> 19;
  pbclk = sysclk >> pbdiv;
  
  /* Multiply by two, this way we can round the divider up if needed */
  uxbrg = ((pbclk * 2) / (divmult * baudrate)) - 2;
  /* We'll get closer if we round up */
  if (uxbrg & 1)
    uxbrg >>= 1, uxbrg++;
  else
    uxbrg >>= 1;
  return uxbrg;
}

void uart_setup() {
  // set up UART1 for 115200 baud
  U1BRG = calculate_baudrate_divider(80000000, 11520, 0);
  U1STA = 0;
  // 8-bit data, no parity, 1 stop bit
  U1MODE = 0x8000;
  // enable transmit and recieve
  U1STASET = 0x1400;	
  
  // enable interrupts for recieve
  /*
  IFSCLR(0) = (1 << 27); // set flag to zero
  IECSET(0) = 1 << 27; // enable
  U1STACLR = (1 << 7) | (1 << 6);  // interrupts when recieving a byte
  IPCSET(6) = 0x1f; // set priority 
  */
}

// return 1 or 0
uint8_t value_in_readbuf() {
 return U1STA & 1;
}

// Recieve one byte via uart
uint8_t uart_read() {
  while (!(U1STA & 0x1)); //wait for read buffer to have a value
  if ((U1STA >> 1) & 1) crash("buffer","overflow","","");
  uint8_t val = U1RXREG & 0xFF;
  //PORTE = val;
  return val;
}

// Check that first 9 is "SCS SENV"
uint8_t envelope_correct(uint8_t* d) {
 return (d[0] == 0x53 && d[1] == 0x43 && d[2] == 0x53 
 && d[3] == 0x20 && d[4] == 0x53 && d[5] == 0x45
 && d[6] == 0x4e && d[7] == 0x56 && d[8] == 0);
}

// recieve SCS protocol envelope
// two first bytes are the length of the data
uint8_t* recv_envelope() {
  uint16_t i;
  // recieve "SCS SENV" (incluing null byte)
  for (i = 0; i < 9; ++i) {
    ubuf[i] = uart_read();
  }
  // if first part is not "SCS SENV"
  //if(!envelope_correct(ubuf)) crash("Invalid","Envelope","","");

  // recieve 8 byte nonce and 2 byte length
  for (i = 9; i < 19; ++i) {
     ubuf[i] = uart_read();
  }
  uint16_t len = ubuf[17];
  len = len << 8;
  len |= ubuf[18];

  // recieve cipher
  for (i = 19; i < 19 + len; ++i) {
    ubuf[i] = uart_read();
  }
  return ubuf;
}

// Send one byte via uart
void uart_write(uint8_t data) {
  while(U1STA & (1 << 9)); //make sure the write buffer is not full 
  //delay(3*2000000);
  //PORTE = data;
  U1TXREG = data;
}

// len is string including null byte
void send_string(uint8_t len, char* s) {
  uint8_t i;
  for (i = 0; i < len; ++i) {
    uart_write(s[i]);
  }
}

// Send an envelope according to the SCS protocol.
// assumes data array is in big endian
// sends len in big endian
void send_envelope(const uint16_t len, const uint8_t nonce[8],
const uint8_t cipher[len]) {
  uint16_t i;
  // send "SCS SENV"
  uart_write(0x53);uart_write(0x43);uart_write(0x53);
  uart_write(0x20);uart_write(0x53);uart_write(0x45);
  uart_write(0x4e);uart_write(0x56);uart_write(0);

  // send nonce
  for (i = 0; i < 8; ++i) {
    uart_write(nonce[i]);
  }

  // send length of envelope in big endian
  uart_write(len >> 8);
  uart_write(len);

  // send data
  for (i = 0; i < len; ++i) {
    uart_write(cipher[i]);
  } 
}


void show_bytes(const uint16_t l, const uint8_t d[l]) {
  char s[l+1];
  s[l] = 0;
  uint16_t i;
  for (i = 0; i < l; ++i) {
    s[i] = d[i];
  }
  display_string(3, s);
  display_update();
}

void show_envelope(const uint8_t row, uint8_t envelope[]) {
  display_string(row, envelope+19);
  display_update();
}

void test_show_envelope() {
  uint16_t l = 0x6;
  uint8_t env[19 + 6] = {
   0x53, 0x43, 0x53, 0x20, 0x49, 0x4e, 0x49, 0x54, 0x00,
   0,0,0,0,0,0,0,0,
   (l >> 8), l,
   0x48, 0x45, 0x4A, 0x48, 0x45, 0x4A
  };
  show_envelope(3, env);
}

/*
* Crypto stuff
*/

// Example key
ChaChaCtx ctx;

// Keys that is actually
uint8_t enc_key[32];
uint8_t dec_key[32];

const uint8_t hard_key[] ={
  0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
  0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
  0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
  0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
};
// global nonce
uint8_t nonce[8];

// generate bascially random stuff
void gen_random(uint16_t len, uint8_t* in) {
  uint8_t i;
  for (i = 0; i < len; ++i) {
    // set  values
    in[i] = TMR2; 
    uint8_t j;
    uint8_t delay = TMR2;
    // wait for a random selected delay
    for (j = 0; j < delay; ++j);
  }
}

void gen_nonce() {
  gen_random(8, nonce);
}

// increments global nonce
void inc_nonce() {
  uint8_t i; 
  for (i = 0; i < sizeof(nonce); ++i) {
    nonce[i] = nonce[i] + 1;
    // continue if it is zero
    if (nonce[i]) break;
  }
}


// works both ways with encryped and decrypted
// put whatever should be transformed in the in arg
void enc(uint8_t in[], const uint32_t len, uint8_t out[], 
const uint8_t non[8], const uint8_t key[32]) {
  chacha_init(&ctx, key, non); 
  /*uint8_t i;
  for (i = 0; i < 64; ++i) {
    PORTE =  
  }*/
  chacha20_enc(&ctx, in, len, out); 
}

void send_enc(const uint16_t len, uint8_t message[len]) {
  uint8_t cbuf[len];
  // increment nonce
  inc_nonce(); 
  // encrypt
  enc(message, len, cbuf, nonce, enc_key);
  // send envelope with cipher
  send_envelope(len, nonce, cbuf);
}

uint8_t mbuf[20];
uint8_t* recv_dec_message() {
  // fetch envelope
  uint8_t* mm = recv_envelope();
  
  // get length from envelope
  uint16_t len = mm[17];
  len = len << 8;
  len |= mm[18];
  uint8_t env_cipher[len];
  uint8_t env_nonce[8];

  uint16_t i;
  // get cipher from envelope
  for (i = 0; i < len; ++i) {
    env_cipher[i] = mm[19+i]; 
  }  

  // get nonce from envelope
  for (i = 0; i < 8; ++i) {
    env_nonce[i] = mm[9 + i];
  }
  // decrypt
  enc(env_cipher,len, mbuf, env_nonce, dec_key);
  return mbuf;
}

void show_message(const uint8_t row, uint8_t message[]) {
  display_string(row, message);
  display_update();
}

/* 
* DH and montgomery stuff
*/

uint8_t correct_init(uint8_t* d) {
 return (d[0] == 0x53 && d[1] == 0x43 && d[2] == 0x53 
 && d[3] == 0x20 && d[4] == 0x49 && d[5] == 0x4e
 && d[6] == 0x49 && d[7] == 0x54 && d[8] == 0);
}

void initiate_contact() {
  uint8_t init[HEADER_LEN];
  uint8_t i;
  for (i = 0; i < HEADER_LEN; ++i) {
    init[i] = uart_read(); 
  } 
  // crash if not correct SCS init 
  if(!correct_init(init)) crash("Incorrect","SCS init","message",""); 
}

DhPubKey dhpubkey;
DhPubKey dhpeerpubkey;
DhPrivKey dhprivkey;
uint8_t my_nonce[32];
uint8_t recv_nonce[32];
uint8_t shared_secret[256];

void gen_key_and_nonce() {
  gen_random(32, my_nonce);
  dh_random_priv_key(&dhprivkey);
  dh_calc_pub_key(&dhprivkey, &dhpubkey);
}
// TODO: fix all magic numbers
void send_key_and_nonce() {
  uint8_t pubbuf[256];
  // convert public key to non-montgomery form
  dh_pub_key_to_bytes(&dhpubkey, pubbuf);

  uint16_t i;
  // Send SCS SKEY with null byte
  send_string(HEADER_LEN, "SCS SKEY");

  // send public key
  for (i = 0; i < 256; ++i) {
    uart_write(pubbuf[i]); 
  }
  // send SCS SNON with null byte
  send_string(HEADER_LEN, "SCS SNON");

  // send dh nonce
  for (i = 0; i < 32; ++i) {
    uart_write(my_nonce[i]); 
  } 
}

uint8_t correct_scs_ckey(uint8_t* d) {
 return (d[0] == 'S' && d[1] == 'C' && d[2] == 'S' 
 && d[3] == ' ' && d[4] == 'C' && d[5] == 'K'
 && d[6] == 'E' && d[7] == 'Y' && d[8] == 0);
}

uint8_t correct_scs_cnon(uint8_t* d) {
 return (d[0] == 'S' && d[1] == 'C' && d[2] == 'S' 
 && d[3] == ' ' && d[4] == 'C' && d[5] == 'N'
 && d[6] == 'O' && d[7] == 'N' && d[8] == 0);
}

// TODO: fix magic numbers!
void recv_nonce_and_key() {
  uint16_t i;
  // check that you got "SCS CKEY"
  uint8_t header[9];
  for (i = 0; i < 9; ++i) {
    header[i] = uart_read(); 
  }
  if (!correct_scs_ckey(header)) crash("SCS CKEY:","wrong head","","");
  
  // put recieved stuff into dhpeerpubkey
  uint8_t peerbuf[256];
  for (i = 0; i < 256; ++i) {
    peerbuf[i] = uart_read(); 
  }

  // check that we got "SCS CNON"
  for (i = 0; i < 9; ++i) {
    header[i] = uart_read(); 
  }
  if (!correct_scs_cnon(header)) crash("SCS CNON:","wrong head","","");

  // put recieved stuff into dhpubkey
  for (i = 0; i < 32; ++i) {
    recv_nonce[i] = uart_read(); 
  }
  // convert recieved bytes into montgomery form
  dh_bytes_to_pub_key(peerbuf,&dhpeerpubkey);
}

void gen_shared() {
  dh_calc_shared_secret(&dhprivkey, &dhpeerpubkey, shared_secret);
}

#define MSG_LEN 320
// TODO: fix magic numbers for shared key and other keys!
void gen_keys() {
  uint16_t i;
  uint8_t appended[MSG_LEN];
  //  CKA = spoch(32, append(NA, NB, SS))
  for (i = 0; i < 32; ++i) {
    appended[i] = recv_nonce[i]; 
  }
  for (i = 32; i < 64; ++i) {
    appended[i] = my_nonce[i-32]; 
  }
  for (i = 64; i < MSG_LEN; ++i) {
    appended[i] = shared_secret[i-64]; 
  }
  spoch(appended, MSG_LEN, 32, dec_key);

  //  CKB = spoch(32, append(NB, NA, SS))
  for (i = 0; i < 32; ++i) {
    appended[i] = my_nonce[i]; 
  }
  for (i = 32; i < 64; ++i) {
    appended[i] = recv_nonce[i-32]; 
  }
  for (i = 64; i < MSG_LEN; ++i) {
    appended[i] = shared_secret[i-64]; 
  }
  spoch(appended, MSG_LEN, 32, enc_key);
}

/*
// Grabs the least sig 4 bits of passed byte and returns hex code.
char nibble_to_hex(u8 nibble)
{
  nibble &= 0x0F;
  if (nibble < 10) {
    return '0' + nibble;
  }

  return 'A' + (nibble-10);
}

// Prints up to the first 8 bytes of the passed array bytes to the first line.
void print_buffer_start(u8 line, u8 bytes[], u8 len)
{
  len = (len == 0) || (len > 8) ? 8 : len;
  char str[17]; // 8 bytes = 16 hex digits + null term.
  str[16] = 0;

  u8 i;
  char c;
  for (i = 0; i < len; ++i) {
    c = nibble_to_hex(bytes[i] >> 4);
    str[i*2] = c;
    c = nibble_to_hex(bytes[i]);
    str[i*2 + 1] = c;
  }

  display_string(line, str);
}

void confirm_shared_secret() {
	display_string(0, "CKA,CKB,SS");
	print_buffer_start(1, dec_key, 0);
  print_buffer_start(2, enc_key, 0);
  print_buffer_start(3, shared_secret, 0);
  display_update();
  PORTE = 0x0F;
  while (1);
}
*/
