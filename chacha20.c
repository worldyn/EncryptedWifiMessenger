#include <stdio.h>
#include "machine_defs.h"
#include "chacha20.h"

// Implemented according to chacha20 specification:
// https://cr.yp.to/chacha/chacha-20080128.pdf
// which refers to salsa20 specification:
// https://cr.yp.to/snuffle/spec.pdf


// TODO: Implement in asm on mips.
// In place left rotate. Rotate word a by n bits.
#define LEFT_ROT(a, n)\
  do {\
    (a) = ((a) << (n)) | ((a) >> (8*sizeof(a)) - n);\
  } while(FALSE);


// Should only be called from ONE_ROW or ONE_COLUMN macro inplementations.
// All args must be mutable variables of type u32.
// Ie: No constants.
// Prefer saving a value in a variable instead of passing ans expression of
// form v[idx].
#define QUARTER_ROUND_INNER(a, b, c, d)\
  (a) += (b); (d) ^= (a); LEFT_ROT(d, 16);\
  (c) += (d); (b) ^= (c); LEFT_ROT(b, 12);\
  (a) += (b); (d) ^= (a); LEFT_ROT(d, 8);\
  (c) += (d); (b) ^= (c); LEFT_ROT(b, 7);

#define QUARTER_ROUND(ia, ib, ic, id, a, b, c, d, words)\
  (a) = words[ia];\
  (b) = words[ib];\
  (c) = words[ic];\
  (d) = words[id];\
  QUARTER_ROUND_INNER(a, b, c, d);\
  words[ia] = (a);\
  words[ib] = (b);\
  words[ic] = (c);\
  words[id] = (d);


#define DOUBLE_ROUND(a, b, c, d, words)\
  do {\
    QUARTER_ROUND(0,  4,  8,  12, a, b, c, d, words);\
    QUARTER_ROUND(1,  5,  9,  13, a, b, c, d, words);\
    QUARTER_ROUND(2,  6,  10, 14, a, b, c, d, words);\
    QUARTER_ROUND(3,  7,  11, 15, a, b, c, d, words);\
    QUARTER_ROUND(0,  5,  10, 15, a, b, c, d, words);\
    QUARTER_ROUND(1,  6,  11, 12, a, b, c, d, words);\
    QUARTER_ROUND(2,  7,  8,  13, a, b, c, d, words);\
    QUARTER_ROUND(3,  4,  9,  14, a, b, c, d, words);\
  } while(FALSE);

// Should be fine regardless of endianness... right?
// This interprets bytes to be in little endian form.
#define BYTES_TO_WORD(w, bytes)\
    do {\
      (w) = 0;\
      (w) |= ((u32)(bytes)[0]) << 0;\
      (w) |= ((u32)(bytes)[1]) << 8;\
      (w) |= ((u32)(bytes)[2]) << 16;\
      (w) |= ((u32)(bytes)[3]) << 24;\
    } while(FALSE);

// a is interpreted as being little endian =>
// the least sig byte of the returned w will be a[0].
u32 little_endian_to_word(const u8 a[4])
{
  u32 w = 0;

  w |= a[3];
  w = w << 8;

  w |= a[2];
  w = w << 8;

  w |= a[1];
  w = w << 8;

  w |= a[0];

  return w;
}

// a is interpreted as being big endian =>
// the most sig byte of the returned w will be a[0].
u32 big_endian_to_word(const u8 a[4])
{
  u32 w = 0;

  w |= a[0];
  w = w << 8;

  w |= a[1];
  w = w << 8;

  w |= a[2];
  w = w << 8;

  w |= a[3];

  return w;
}

// Writes bytes of w to a. Least sig byte of w will be written to a[0].
// Ie, a will rep the same word as w in little endian form.
void word_to_little_endian(u32 w, u8 a[4])
{
  a[0] = (u8) w;
  w = w >> 8;

  a[1] = (u8) w;
  w = w >> 8;

  a[2] = (u8) w;
  w = w >> 8;

  a[3] = (u8) w;
}

// Writes bytes of w to a. Most sig byte of w will be written to a[0].
// Ie, a will rep the same word as w in big endian form.
void word_to_big_endian(u32 w, u8 a[4])
{
  a[3] = (u8) w;
  w = w >> 8;

  a[2] = (u8) w;
  w = w >> 8;

  a[1] = (u8) w;
  w = w >> 8;

  a[0] = (u8) w;
}

// Applies 20 rounds (10 double rounds) and adds res to init input. The
// resulting matrix is stored in ctx_out.
void chacha20m(const ChaChaCtx* ctx_in, ChaChaCtx* ctx_out)
{
  register u32 a;
  register u32 b;
  register u32 c;
  register u32 d;
  register u8 k;

  // Copy ctx into tmp.
  for (k = 16; k-- > 0;) {
    ctx_out->words[k] = ctx_in->words[k];
  }

  // Apply 10 double rounds to tmp.
  for (k = 10; k-- > 0;) {
    DOUBLE_ROUND(a, b, c, d, (ctx_out->words));
  }

  for (k = 16; k-- > 0;) {
    ctx_out->words[k] += ctx_in->words[k];
  }
}

// Applies 20 rounds (10 double rounds) using ctx as inp, stores res in out.
// ctx is not mutated in any way, ctr is _not_ increased.
void chacha20(const ChaChaCtx* ctx, u8 out[])
{
  u8 k;
  ChaChaCtx tmp;
  chacha20m(ctx, &tmp);
  for (k = 0; k < 16; ++k) {
    word_to_little_endian(tmp.words[k], out+(4*k));
  }
}

// Sets all word in ctx to 0.
void chacha_set_zero(ChaChaCtx* ctx)
{
  u8 i;
  for (i = 16; i-- > 0;) {
    ctx->words[i] = 0;
  }
}

// Sets words 0..3 to the constants defined by the spec (same as salsa).
void chacha_set_consts(ChaChaCtx* ctx)
{
  // TODO: Think about endianess! Maybe harcode as ints instead?
  // Set constant to ascii "expand 32-byte k"
  // Ie: 16 bytes -> 4 words.
  ctx->words[0] = ((u32)'e') | ((u32)'x'<<8) | ((u32)'p'<<16) | ((u32)'a'<<24);
  ctx->words[1] = ((u32)'n') | ((u32)'d'<<8) | ((u32)' '<<16) | ((u32)'3'<<24);
  ctx->words[2] = ((u32)'2') | ((u32)'-'<<8) | ((u32)'b'<<16) | ((u32)'y'<<24);
  ctx->words[3] = ((u32)'t') | ((u32)'e'<<8) | ((u32)' '<<16) | ((u32)'k'<<24);
}

// nonce is assumed to be len 8 bytes.
void chacha_set_nonce(ChaChaCtx* ctx, const u8 nonce[])
{
  BYTES_TO_WORD(ctx->words[14], &nonce[0]);
  BYTES_TO_WORD(ctx->words[15], &nonce[4]);
}

// extract nonce. writes it in little endian form.
void chacha_extract_nonce(const ChaChaCtx* ctx, u8 nonce[8])
{
  word_to_little_endian(ctx->words[14], nonce);
  word_to_little_endian(ctx->words[15], nonce+4);
}


// The ctr is just 8 words = 256 bits
// key is assumed to be len 32 bytes.
void chacha_set_key(ChaChaCtx* ctx, /*const u32 key[]*/ const u8 key[])
{
  /*
  ctx->words[4]  = key[0];
  ctx->words[5]  = key[1];
  ctx->words[6]  = key[2];
  ctx->words[7]  = key[3];
  ctx->words[8]  = key[4];
  ctx->words[9]  = key[5];
  ctx->words[10] = key[6];
  ctx->words[11] = key[7];
  */
  BYTES_TO_WORD(ctx->words[4],  &key[0]);
  BYTES_TO_WORD(ctx->words[5],  &key[4]);
  BYTES_TO_WORD(ctx->words[6],  &key[8]);
  BYTES_TO_WORD(ctx->words[7],  &key[12]);
  BYTES_TO_WORD(ctx->words[8],  &key[16]);
  BYTES_TO_WORD(ctx->words[9],  &key[20]);
  BYTES_TO_WORD(ctx->words[10], &key[24]);
  BYTES_TO_WORD(ctx->words[11], &key[28]);
}

// Extract ctr from ctx as two u32s, a is least sig, b is most sig.
void chacha_extract_ctr(const ChaChaCtx* ctx, u32* a, u32* b)
{
  u32 w;

  // Least sig word is word 12 (bytes 48..51)
  w = ctx->words[12];
  *a = 0;
  *a |= (u32) ((u8)(w >> 24));
  *a = *a << 8;
  *a |= (u32) ((u8)(w >> 16));
  *a = *a << 8;
  *a |= (u32) ((u8)(w >> 8));
  *a = *a << 8;
  *a |= (u32) ((u8)w);

  // Most sig word is word 13 (bytes 42..55)
  w = ctx->words[13];
  *b = 0;
  *b |= (u32) ((u8)(w >> 24));
  *b = *b << 8;
  *b |= (u32) ((u8)(w >> 16));
  *b = *b << 8;
  *b |= (u32) ((u8)(w >> 8));
  *b = *b << 8;
  *b |= (u32) ((u8)w);
}

// The passed ctr_bytes are assumed to be in little endian.
void chacha_set_ctr_bytes(ChaChaCtx* ctx, const u8 ctr_bytes[8])
{
  u32 w;
  w = little_endian_to_word(ctr_bytes);
  ctx->words[12] = w;
  w = little_endian_to_word(ctr_bytes+4);
  ctx->words[13] = w;
}



void chacha_set_ctr(ChaChaCtx* ctx, u32 a, u32 b)
{
  u32 w;

  w = 0;
  w |= (u32) ((u8)(a >> 24));
  w = w << 8;
  w |= (u32) ((u8)(a >> 16));
  w = w << 8;
  w |= (u32) ((u8)(a >> 8));
  w = w << 8;
  w |= (u32) ((u8)a);
  ctx->words[12] = w;

  w = 0;
  w |= (u32) ((u8)(b >> 24));
  w = w << 8;
  w |= (u32) ((u8)(b >> 16));
  w = w << 8;
  w |= (u32) ((u8)(b >> 8));
  w = w << 8;
  w |= (u32) ((u8)b);
  ctx->words[13] = w;
}

// Increases the ctr of ctx by 1.
void chacha_inc_ctr(ChaChaCtx* ctx)
{
  u32 a, b;
  chacha_extract_ctr(ctx, &a, &b);

  ++a;
  if (a == 0) { // overflow of least sig word, inc b.
    ++b;
  }

  chacha_set_ctr(ctx, a, b);
}

// Sets the constant words in ctx correctly.
// Sets the key words in ctx according to the key arg.
// Sets the nonce words in ctx according to the nonce arg.
// Sets ctr in ctx to 0.
void chacha_init(ChaChaCtx* ctx, const u8 key[], const u8 nonce[])
{
  chacha_set_consts(ctx);
  chacha_set_key(ctx, key);
  chacha_set_nonce(ctx, nonce);
  chacha_set_ctr(ctx, 0, 0);
}

// Tests from
// https://tools.ietf.org/html/draft-agl-tls-chacha20poly1305-04#section-7

// First with all zero key and all zero nonce.
void chacha_init_test1(ChaChaCtx* ctx)
{
  chacha_set_zero(ctx);

  chacha_set_consts(ctx);

  // Key is already zero.

  // The last 4 words are "input" as in ctr and nonce. Since both nonce
  // and ctr are 0, all are set to 0.
  u8 nonce[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  chacha_set_nonce(ctx, nonce);
  chacha_set_ctr(ctx, 0, 0);
}

// Second with all zero key except 1 least sig bit and all zero nonce.
void chacha_init_test2(ChaChaCtx* ctx)
{
  chacha_set_zero(ctx);

  chacha_set_consts(ctx);

  u8 key[] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01,
  };
  chacha_set_key(ctx, key);

  // The last 4 words are "input" as in ctr and nonce. Since both nonce
  // and ctr are 0, all are set to 0.
  u8 nonce[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  chacha_set_nonce(ctx, nonce);
  chacha_set_ctr(ctx, 0, 0);
}

// Third with all zero key and all zero nonce except least sig bit set to 1.
void chacha_init_test3(ChaChaCtx* ctx)
{
  chacha_set_zero(ctx);

  chacha_set_consts(ctx);

  // Key is alread 0.

  u8 nonce[] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01,
  };
  chacha_set_nonce(ctx, nonce);
  chacha_set_ctr(ctx, 0, 0);
}


// Fourth test. Key = 0, nonce = 0x0100000000000000
void chacha_init_test4(ChaChaCtx* ctx)
{
  chacha_set_zero(ctx);

  chacha_set_consts(ctx);

  // Key is alread 0.

  u8 nonce[] = {
    0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
  };
  chacha_set_nonce(ctx, nonce);
  chacha_set_ctr(ctx, 0, 0);
}

// Fifth test.
// Key   = 0x000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f
// nonce = 0x0001020304050607
void chacha_init_test5(ChaChaCtx* ctx)
{
  chacha_set_zero(ctx);

  chacha_set_consts(ctx);

  u8 key[] = {
   0x00,0x01,0x02,0x03,
   0x04,0x05,0x06,0x07,
   0x08,0x09,0x0a,0x0b,
   0x0c,0x0d,0x0e,0x0f,
   0x10,0x11,0x12,0x13,
   0x14,0x15,0x16,0x17,
   0x18,0x19,0x1a,0x1b,
   0x1c,0x1d,0x1e,0x1f,
  };
  chacha_set_key(ctx, key);

  u8 nonce[] = {
    0x00,0x01,0x02,0x03,
    0x04,0x05,0x06,0x07,
  };
  chacha_set_nonce(ctx, nonce);
  chacha_set_ctr(ctx, 0, 0);
}

// Assumes len is multiple of 64.
void chacha20_keystream(ChaChaCtx* ctx, u32 len, u8 res[])
{
  u32 key_idx;
  for (key_idx = 0; key_idx < len; key_idx += 64) {
    chacha20(ctx, res + key_idx);
    chacha_inc_ctr(ctx);
  }

}

// Encrypt (or decrypt, same operation) len bytes in 'in' and store res in 'out'.
// Assumes ctx has been init with key, nonce and constants.
// Sets ctr to 0 at start and sets back to 0 afterwards.
// TODO: Super stupid with the keystream...
void chacha20_enc(ChaChaCtx* ctx, const u8 in[], u32 len, u8 out[])
{
  // Do nothing if len == 0.
  if (!len) { return; }

  // set stream_len to len rounded up to closest multiple of 64.
  const u32 stream_len = (((len-1)/64)+1) * 64;
  u8 keystream[stream_len];

  chacha_set_ctr(ctx, 0, 0);
  chacha20_keystream(ctx, stream_len, keystream);

  u32 i;
  for (i = 0; i < len; ++i) {
    out[i] = keystream[i] ^ in[i];
  }

  chacha_set_ctr(ctx, 0, 0);
}




