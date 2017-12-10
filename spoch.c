#include "chacha20.h"
#include <stdio.h>
#include <string.h>


// The transformation function F.
// Does an inplace transformation of the passed state s.
static void transform(ChaChaCtx* s)
{
  ChaChaCtx temp;
  chacha20m(s, &temp);
  chacha20m(&temp, s);
}

static void derive_iv(u32 dgst_len, ChaChaCtx* iv_out)
{
  u8 p[] = {
    0x02, 0x03, 0x05, 0x07,
    0x0B, 0x0D, 0x11, 0x13,
    0x17, 0x1D, 0x1F, 0x25,
    0x29, 0x2B, 0x2F, 0x35,
    0x3B, 0x3D, 0x43, 0x47,
    0x49, 0x4F, 0x53, 0x59,
    0x61, 0x65, 0x67, 0x6B,
    0x6D, 0x71, 0x7F, 0x83,
  };

  // Init ctr to 0
  u8 ctr[] = {
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
  };
  word_to_little_endian(dgst_len, ctr+0);

  // Set nonce to 0
  u8 nonce[] = {
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
  };

  chacha_set_consts(iv_out);
  chacha_set_key(iv_out, p);
  chacha_set_nonce(iv_out, nonce);
  chacha_set_ctr_bytes(iv_out, ctr);

  transform(iv_out);
}

#define MSG_LEN_TO_PADDED_LEN(l) ( ((l/8)+1)*8  )

// Writes the padded version of msg to padded_msg.
// (Pad to multiple of 8 bytes, see spec).
// padded_msg is exprected to be of length:
//    MSG_LEN_TO_PADDED_LEN(msg_len)
static void pad_msg(const u8 msg[], u32 msg_len, u8 padded_msg[])
{
  u32 i;
  for (i = 0; i < msg_len; ++i) {
    padded_msg[i] = msg[i];
  } 

  i = msg_len;

  // Scenario 1, only one byte padding needed.
  if (msg_len % 8 == 7) {
    padded_msg[i] = 0x81;
    return;
  }

  // Scenario 2:
  padded_msg[i++] = 0x80;
  u8 zeros_needed = 7 - ((msg_len+1) % 8);
  while (zeros_needed-- > 0) {
    padded_msg[i++] = 0x00;
  }
  padded_msg[i] = 0x01;
}

void spoch(const uint8_t msg[], uint32_t msg_len, uint32_t dgst_len,
  uint8_t dgst_out[]) {
  ChaChaCtx s;
  derive_iv(dgst_len, &s);

  const u32 padded_len = MSG_LEN_TO_PADDED_LEN(msg_len);
  u8 padded_msg[padded_len];
  pad_msg(msg, msg_len, padded_msg);

  u32 i, j;
  u8 nonce[8];

  // Absorption phase:
  for (i = 0; i < padded_len; i+=8) {
    // Read 8 bytes from msg and xor onto R (nonce words) of S:
    // First extract current R (nonce words).
    chacha_extract_nonce(&s, nonce);
    // XOR the next msg bytes onto the nonce.
    for (j = 0; j < 8; ++j) {
      nonce[j] ^= padded_msg[i+j];
    }
    // Write the new nonce back to S.
    chacha_set_nonce(&s, nonce);

    // Perform transformation (inplace):
    transform(&s);
  }

  // Extraction/squeeze phase:
  for (i = 0; i < dgst_len;) {
    u8 bytes_to_grab = 8;
    if (dgst_len-i < 8) {
      bytes_to_grab = dgst_len - i;
    }
    chacha_extract_nonce(&s, nonce);
    for (j = 0; j < bytes_to_grab; ++j) {
      dgst_out[i+j] = nonce[j];
    }

    i += bytes_to_grab;

    transform(&s);
  }
}


// Performs produces an hmac of the msg using spoch of digest length dgst_len.
// The size of mac_out is assumed to be the same as dgst_len.
void spoch_hmac(const u8 msg[], u32 msg_len, const u8 key[], u32 key_len,
                                               u32 dgst_len, u8 mac_out[])
{
  u32 i;

  // Looking at the wiki definition of hmac, we keep K' as K.
  // So key is used directly without being padded with zeros or going through
  // spoch first.

  // key XOR ipad
  u8 inner_key[key_len];
  for (i = 0; i < key_len; ++i) {
    inner_key[i] = key[i] ^ 0x36;
  }

  //key XOR opad
  u8 outer_key[key_len];
  for (i = 0; i < key_len; ++i) {
    outer_key[i] = key[i] ^ 0x5c;
  }

  // concat(inner_key, msg)
  u8 msg_and_key[key_len + msg_len];
  for (i = 0; i < key_len; ++i) {
    msg_and_key[i] = inner_key[i];
  }
  for (i = 0; i < msg_len; ++i) {
    msg_and_key[i+key_len] = msg[i];
  }

  // spoch(concat(inner_key, msg)
  u8 inner_dgst[dgst_len];
  spoch(msg_and_key, key_len+msg_len, dgst_len, inner_dgst);

  // concat(outer_key, inner_dgst)
  u8 inner_msg[key_len + dgst_len];
  for (i = 0; i < key_len; ++i) {
    inner_msg[i] = outer_key[i];
  }
  for (i = 0; i < dgst_len; ++i) {
    inner_msg[i+key_len] = inner_dgst[i];
  }

  // Outer digest: spoch(concat(outer_keu, inner_dgst))
  spoch(inner_msg, key_len+dgst_len, dgst_len, mac_out);
}

