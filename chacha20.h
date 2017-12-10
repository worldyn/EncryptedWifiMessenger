#ifndef CHACHA20_H__
#define CHACHA20_H__

#include <stdio.h>
#include "machine_defs.h"


typedef struct
{
  u32 words[16];
} ChaChaCtx;

/**************************************************************************
 * little_endian_to_word:                                                 *
 * a[0] = 0xAA, a[1] = 0xBB, a[2] = 0xCC, a[3] = 0xDD  =>  w = 0xDDCCBBAA *
 *                                                                        *
 * big_endian_to_word:                                                    *
 * a[0] = 0xAA, a[1] = 0xBB, a[2] = 0xCC, a[3] = 0xDD  =>  w = 0xAABBCCDD *
 *                                                                        *
 * word_to_little_endian:                                                 *
 * w = 0xAABBCCDD  =>  a[0] = 0xDD, a[1] = 0xCC, a[2] = 0xBB, a[3] = 0xAA *
 *                                                                        *
 * little_endian_to_word:                                                 *
 * w = 0xAABBCCDD  =>  a[0] = 0xAA, a[1] = 0xBB, a[2] = 0xCC, a[3] = 0xDD *
 **************************************************************************/


// a is interpreted as being little endian =>
// the least sig byte of the returned w will be a[0].
u32 little_endian_to_word(const u8 a[4]);

// a is interpreted as being big endian =>
// the most sig byte of the returned w will be a[0].
u32 big_endian_to_word(const u8 a[4]);

// Writes bytes of w to a. Least sig byte of w will be written to a[0].
// Ie, a will rep the same word as w in little endian form.
void word_to_little_endian(u32 w, u8 a[4]);

// Writes bytes of w to a. Most sig byte of w will be written to a[0].
// Ie, a will rep the same word as w in big endian form.
void word_to_little_endian(u32 w, u8 a[4]);

// Applies 20 rounds (10 double rounds) and adds res to init input. The
// resulting matrix is stored in ctx_out.
void chacha20m(const ChaChaCtx* ctx_in, ChaChaCtx* ctx_out);


// Applies 20 rounds (10 double rounds) using ctx as inp, stores res in out.
// ctx is not mutated in any way, ctr is _not_ increased.
void chacha20(const ChaChaCtx* ctx, u8 out[]);

// Sets all word in ctx to 0.
void chacha_set_zero(ChaChaCtx* ctx);

// Sets words 0..3 to the constants defined by the spec (same as salsa).
void chacha_set_consts(ChaChaCtx* ctx);

// nonce is assumed to be len 8 bytes and in little endian.
void chacha_set_nonce(ChaChaCtx* ctx, const u8 nonce[8]);

// extract nonce. writes it in little endian form.
void chacha_extract_nonce(const ChaChaCtx* ctx, u8 nonce[8]);

// key is assumed to be len 32 bytes in little endian.
void chacha_set_key(ChaChaCtx* ctx, /*const u32 key[]*/ const u8 key[32]);

// Extract ctr from ctx as two u32s, a is least sig, b is most sig.
void chacha_extract_ctr(const ChaChaCtx* ctx, u32* a, u32* b);

void chacha_set_ctr(ChaChaCtx* ctx, u32 a, u32 b);

// Increases the ctr of ctx by 1.
void chacha_inc_ctr(ChaChaCtx* ctx);

// The passed ctr_bytes are assumed to be in little endian.
void chacha_set_ctr_bytes(ChaChaCtx* ctx, const u8 ctr_bytes[8]);


// Sets the constant words in ctx correctly.
// Sets the key words in ctx according to the key arg.
// Sets the nonce words in ctx according to the nonce arg.
// Sets ctr in ctx to 0.
void chacha_init(ChaChaCtx* ctx, const u8 key[], const u8 nonce[]);


// Assumes len is multiple of 64.
void chacha20_keystream(ChaChaCtx* ctx, u32 len, u8 res[]);

// Encrypt (or decrypt, same operation) len bytes in 'in' and store res in 'out'.
// Sets ctr to 0 at start and sets back to 0 afterwards.
// TODO: Super stupid with the keystream...
void chacha20_enc(ChaChaCtx* ctx, const u8 in[], u32 len, u8 out[]);

#endif // CHACHA20_H__
