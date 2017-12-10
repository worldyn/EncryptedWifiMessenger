#include <stdio.h>
#include "dh.h"
#include <pic32mx.h>

static const MontgomeryCtx* ctx = &mctx;

uint32_t random_word() {
  // wait for timer to run a short while
  uint8_t delay = TMR2;
  uint8_t j;
  for (j = 0; j < delay; ++j);

  uint32_t x = TMR2;
  x = x << 16;
  x |= TMR2;
  return x;
}

// Random private key of length R_LEN.
void dh_random_priv_key(DhPrivKey* priv) {
  uint32_t i;
  for (i = 0; i < PRIV_LEN; ++i) {
    priv->limbs[i] = random_word();
  }
}


void dh_pub_key_to_bytes(const DhPubKey* pub, uint8_t* bytes) {
  Limb x[R_LEN];
  bigu_from_mont(pub->limbs, ctx, x);
  bigu_into_bytes(x, R_LEN, bytes);
}

void dh_bytes_to_pub_key(const uint8_t* bytes, DhPubKey* pub) {
  Limb x[R_LEN];
  bigu_from_bytes(bytes, R_LEN, x);
  bigu_into_mont(x, ctx, pub->limbs);
}


// Assumes priv and pub are have length R_LEN.
// Calculates a public key with a private key given.
void dh_calc_pub_key(const DhPrivKey* priv, DhPubKey* pub) {
  // pub = g ^ priv (mod n)
  bigu_mont_pow(gm, priv->limbs, PRIV_LEN, ctx, pub->limbs);
}

// Calculate shared secret from private key and peer's pub key
// lenght of shared secret will be R_LEN * LIMB_BYTE_COUNT.
void dh_calc_shared_secret(const DhPrivKey* priv, const DhPubKey* pub,
  uint8_t* ss) {
  Limb x[R_LEN];
  Limb y[R_LEN];

  // Perform actual calc. Store res in x.
  bigu_mont_pow(pub->limbs, priv->limbs, PRIV_LEN, ctx, x);
  // Res is in montgomery form. Covert out of it. Store new form in y.
  bigu_from_mont(x, ctx, y);
  // Write y to ss in byte array big endian form.
  bigu_into_bytes(y, R_LEN, ss);
}


