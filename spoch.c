#include "chacha20_2.h"

void padding(uint8_t msg[], uint32_t len, uint8_t out[]) {
  uint32_t i;
  // copy msg to out
  for (i = 0; i < len; ++i) {
    out[i] = msg[i]; 
  }

  if (len % 8 == 7) {
    // append one byte 0x81
    out[len] = 0x81;
  } else {
    out[len] = 0x80; 
    uint32_t needed_zeroes = 7 - ((len + 1) % 8);
    for (i = 0; i < needed_zeroes; ++i) {
      out[len + 1 + i] = 0;
    }
    out[len + 1 + needed_zeroes] = 1;
  }
}

void transform(ChaChaCtx* ctx) {
 ChaChaCtx temp_state;   
 chacha20_hash(ctx, &temp_state);
 chacha20_hash(&temp_state, ctx);
}

void iv(ChaChaCtx* ctx, uint32_t len) {
  uint8_t iv_key[] = {
    0x02, 0x03, 0x05, 0x07,
    0x0B, 0x0D, 0x11, 0x13,
    0x17, 0x1D, 0x1F, 0x25,
    0x29, 0x2B, 0x2F, 0x35,
    0x3B, 0x3D, 0x43, 0x47,
    0x49, 0x4F, 0x53, 0x59,
    0x61, 0x65, 0x67, 0x6B,
    0x6D, 0x71, 0x7F, 0x83,
  }; 
  uint8_t iv_nonce[] = {
    0,0,0,0,0,0,0,0
  };
  chacha20_expand_init(ctx,iv_nonce,iv_key);	
  chacha_set_matrix_counter(ctx, 0, len);
  transform(ctx);
}

void spoch(uint8_t msg[], uint32_t msg_len, uint32_t digest_len,
  uint8_t out[digest_len]) {
  // STart by padding message
  // next multiple of eight for msg_len + 1
  // must always pad at least one byte
  uint32_t padded_msg_len = ((msg_len+2)/8 + 1)*8; 
  uint8_t padded_msg[padded_msg_len];
  padding(msg, msg_len,padded_msg);
  
  ChaChaCtx state;
  // initialization of state
  iv(&state, digest_len);
  
  // Process input accordign to SPOCH protocol
  uint32_t i;
  uint8_t state_nonce[8];
  uint32_t j;
  for (i = 0; i < padded_msg_len; i+=8) {
    chacha_get_nonce(&state, state_nonce); 
    for (j = 0; j < 8; ++j) {
      state_nonce[j] ^= padded_msg[i+j]; 
    } 
    chacha_set_nonce(&state, state_nonce);
    transform(&state);
  }

  // Process output according to SPOCH protocol
  for (i = 0; i < digest_len;) {
    chacha_get_nonce(&state, state_nonce); 
    for (j = 0; j < 8 && i < digest_len; ++i, ++j) {
      out[i] = state_nonce[j];
    }
    transform(&state);
  } 
}
