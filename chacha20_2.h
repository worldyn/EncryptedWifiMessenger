#include <stdint.h>
// Implemented via salsa20 speci. with chacha20 spec:
// https://cr.yp.to/snuffle/spec.pdf
// https://cr.yp.to/chacha/chacha-20080128.pdf
typedef struct {
  uint32_t matrix[16];
} ChaChaCtx;

void chacha20_hash(const ChaChaCtx* ctx, ChaChaCtx* out);
void chacha20_expand_init(ChaChaCtx* ctx, const uint8_t nonce[],
  const uint8_t key[]);
void chacha20_enc(ChaChaCtx* ctx, const uint8_t in[], uint32_t len,
  uint8_t out[]);

void bytes_to_word(uint32_t* word, const uint8_t bytes[]);
void word_to_little_endian(uint32_t w, uint8_t a[4]);
void chacha_get_nonce(const ChaChaCtx* ctx, uint8_t nonce[8]);
void chacha_set_nonce(ChaChaCtx* ctx, const uint8_t nonce[]);
void ctx_to_bytes(const ChaChaCtx* ctx, uint8_t bytes[]);
void chacha_set_matrix_counter(ChaChaCtx* ctx, uint32_t cnt_most_sig, 
  uint32_t cnt_least_sig);
