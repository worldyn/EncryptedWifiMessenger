#ifndef COMMUNICATION_H__
#define COMMUNIACTION_H__

#define ENVELOPE_SIZE 4095
void initiate_contact();
uint8_t ubuf[ENVELOPE_SIZE]; 
void uart_setup();
uint8_t uart_read();
uint8_t* recv_envelope();
void uart_write(uint8_t data);
void send_envelope(const uint16_t len, const uint8_t nonce[8],
  const uint8_t cipher[len]);
void show_envelope(const uint8_t row, uint8_t envelope[]);
void test_show_envelope();
void send_enc(const uint16_t len, uint8_t message[len]);
uint8_t* recv_enc();
void show_bytes(const uint16_t l,const uint8_t* d);
uint8_t value_in_readbuf();
void show_message(const uint8_t row, uint8_t message[]);
uint8_t* recv_dec_message();
void gen_nonce();
void send_key_and_nonce();
void gen_key_and_nonce();
void recv_nonce_and_key();
void gen_shared();
void gen_keys();
void confirm_shared_secret();
#endif // COMMUNICATION_H__


