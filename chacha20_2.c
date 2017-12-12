#include "chacha20_2.h"
#include "crash.h"
#define ROUNDS 10

// Implemented via salsa20 speci. with chacha20 spec:
// https://cr.yp.to/snuffle/spec.pdf
// https://cr.yp.to/chacha/chacha-20080128.pdf

// https://blog.regehr.org/archives/1063
uint32_t rot32(uint32_t x, uint32_t n) {
  return (x<<n) | (x>>(32-n));
}

void quarterround(uint32_t* a, uint32_t* b, uint32_t* c, uint32_t* d) {
  (*a) += (*b); (*d) ^= (*a); (*d) = rot32((*d),16);
  (*c) += (*d); (*b) ^= (*c); (*b) = rot32((*b), 12);
  (*a) += (*b); (*d) ^= (*a); (*d) = rot32((*d),8);
  (*c) += (*d); (*b) ^= (*c); (*b) = rot32((*b),7);
}

void columnround(ChaChaCtx* ctx) {
  quarterround(&(ctx->matrix[0]), &(ctx->matrix[4]), 
    &(ctx->matrix[8]),&(ctx->matrix[12]));
  quarterround(&(ctx->matrix[1]), &(ctx->matrix[5]), 
    &(ctx->matrix[9]), &(ctx->matrix[13]));
  quarterround(&(ctx->matrix[2]), &(ctx->matrix[6]),
    &(ctx->matrix[10]), &(ctx->matrix[14]));
  quarterround(&(ctx->matrix[3]), &(ctx->matrix[7]), 
    &(ctx->matrix[11]), &(ctx->matrix[15]));
}

void diagonalround(ChaChaCtx* ctx) {
  quarterround(&(ctx->matrix[0]), &(ctx->matrix[5]), 
    &(ctx->matrix[10]), &(ctx->matrix[15]));
  quarterround(&(ctx->matrix[1]), &(ctx->matrix[6]), 
    &(ctx->matrix[11]), &(ctx->matrix[12]));
  quarterround(&(ctx->matrix[2]), &(ctx->matrix[7]), 
    &(ctx->matrix[8]), &(ctx->matrix[13]));
  quarterround(&(ctx->matrix[3]), &(ctx->matrix[4]), 
    &(ctx->matrix[9]), &(ctx->matrix[14]));
}

void doubleround(ChaChaCtx* ctx) {
  columnround(ctx);
  diagonalround(ctx);
}

//Hash function
//in and out is 64 bytes
void chacha20_hash(const ChaChaCtx* ctx, ChaChaCtx* out) {
  // Copy input
  uint8_t i;
  for(i = 0; i < 16; ++i) {
    out->matrix[i] = ctx->matrix[i]; 
  }
  
  // perform the double rounds
  for(i = 0; i < ROUNDS; ++i) {
    doubleround(out);
  } 

  // add all elements in mat and in into out
  for(i = 0; i < 16; ++i) {
    out->matrix[i] += ctx->matrix[i]; 
  }
}


void bytes_to_word(uint32_t* word, const uint8_t bytes[]) {
  *word = 0;
  *word |= (uint32_t)(bytes[0]) << 0;
  *word |= (uint32_t)(bytes[1]) << 8;
  *word |= (uint32_t)(bytes[2]) << 16;
  *word |= (uint32_t)(bytes[3]) << 24;
}

void word_to_little_endian(uint32_t w, uint8_t a[4]) {
  a[0] = (uint8_t) w;
  w = w >> 8;

  a[1] = (uint8_t) w;
  w = w >> 8;

  a[2] = (uint8_t) w;
  w = w >> 8;

  a[3] = (uint8_t) w;
}

void chacha20_expand_init(ChaChaCtx* ctx, const uint8_t nonce[],
  const uint8_t key[]) {
  // Constants, byte 0-3
  // consts is "expand 32-byte k" in ascii to hex
  ctx->matrix[0] = 0x61707865;
  ctx->matrix[1] = 0x3320646E;
  ctx->matrix[2] = 0x79622D32; 
  ctx->matrix[3] = 0x6B206574;
  // Key, byte 4-11
  bytes_to_word(&(ctx->matrix[4]), &key[0]);
  bytes_to_word(&(ctx->matrix[5]), &key[4]);
  bytes_to_word(&(ctx->matrix[6]), &key[8]);
  bytes_to_word(&(ctx->matrix[7]), &key[12]);
  bytes_to_word(&(ctx->matrix[8]), &key[16]);
  bytes_to_word(&(ctx->matrix[9]), &key[20]);
  bytes_to_word(&(ctx->matrix[10]), &key[24]);
  bytes_to_word(&(ctx->matrix[11]), &key[28]);
  // Counter, byte 12-13
  ctx->matrix[12] = 0; 
  ctx->matrix[13] = 0;
  // Nonce, bytes, 14-15
  bytes_to_word(&(ctx->matrix[14]), &nonce[0]); 
  bytes_to_word(&(ctx->matrix[15]), &nonce[4]);
}

void chacha_reset_matrix(ChaChaCtx* ctx) {
  uint8_t i;
  for(i = 0; i < 16; ++i) {
    ctx->matrix[i] = 0;
  }
}

void chacha_inc_matrix_counter(ChaChaCtx* ctx) {
  ctx->matrix[12]++;  
  // if overflow
  if (ctx->matrix[12] == 0) {
    ctx->matrix[13]++;  
  }
}

// count in little endian
void chacha_set_matrix_counter(ChaChaCtx* ctx, uint32_t cnt_most_sig, 
  uint32_t cnt_least_sig) {
  ctx->matrix[12] = cnt_least_sig;  
  ctx->matrix[13] = cnt_most_sig;
}

// little endian according to specification
void ctx_to_bytes(const ChaChaCtx* ctx, uint8_t bytes[]) {
  uint8_t i;
  uint8_t j;
  for (i = 0, j = 0; i < 16; ++i, j+=4) {
    bytes[j] = ctx->matrix[i]; 
    bytes[j+1] = ctx->matrix[i] >> 8; 
    bytes[j+2] = ctx->matrix[i] >> 16; 
    bytes[j+3] = ctx->matrix[i] >> 24; 
  }
}

//Encryption function
//encrypt and decrypt is same operation
//in = message or ciphertext,
//ctx = chacha20 block/matrix
//
//Assumes that matrix has been initialized first with key and nonce!!!!

void chacha20_enc(ChaChaCtx* ctx, const uint8_t in[], uint32_t len, uint8_t out[]) {
 /* 
  // Debug stuff
  print_buffer_start(0, (uint8_t*)(ctx->matrix) + 0, 0);
  print_buffer_start(1, (uint8_t*)(ctx->matrix) + 8, 0);
  print_buffer_start(2, (uint8_t*)(ctx->matrix) + 16, 0);
  print_buffer_start(3, (uint8_t*)(ctx->matrix) + 24, 0);
  display_update();
 */ 

  if(len == 0) {
    return;
  }
  
  uint32_t i;
  uint32_t j;

  // Size is rounded up to nearest 64 multiple of len
  // To get 64 bytes per hashed matrix
  uint32_t buf_size = (((len-1)/64) + 1)*64;
  
  uint8_t buffer[buf_size];
  ChaChaCtx bufctx;
    
  for(i = 0; i < buf_size; i+=64) {
    // Perform chacha20 on current matrix
    // Output will be in buffer byte array
    // the 64 bytes written to it
    chacha20_hash(ctx, &bufctx);
    ctx_to_bytes(&bufctx, buffer + i);
    
    
    // Increment counter in matrix
    ctx->matrix[12] = ctx->matrix[12] + 1;
    // if overflow then add one to most significant bytes of counter
    if(ctx->matrix[12] == 0) {
      ctx->matrix[13]++;
    }
  }
  
  // The XOR part as described in algo specification
  for(i = 0; i < len; ++i) {
    out[i] = in[i] ^ buffer[i]; 
  }

  // Reset counter
  ctx->matrix[12] = 0;
  ctx->matrix[13] = 0;
}

// extract nonce. writes it in little endian form.
void chacha_get_nonce(const ChaChaCtx* ctx, uint8_t nonce[8]) {
  word_to_little_endian(ctx->matrix[14], nonce);
  word_to_little_endian(ctx->matrix[15], nonce+4);
}

void chacha_set_nonce(ChaChaCtx* ctx, const uint8_t nonce[]) {
  bytes_to_word(&(ctx->matrix[14]), &nonce[0]);
  bytes_to_word(&(ctx->matrix[15]), &nonce[4]);
}
