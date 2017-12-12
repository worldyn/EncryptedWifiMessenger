// Borrowed from Olle
#ifndef DH_H__
#define DH_H__

#include "bigu.h"
#include "dh-params.h"

// A bigu in montgomery form.
typedef struct {
  Limb limbs[R_LEN];
} DhPubKey;

// A bigu not in montgomery form.
typedef struct {
  Limb limbs[PRIV_LEN];
} DhPrivKey;

void dh_random_priv_key(DhPrivKey* priv);
void dh_pub_key_to_bytes(const DhPubKey* pub, uint8_t* bytes);
void dh_bytes_to_pub_key(const uint8_t* bytes, DhPubKey* pub);
void dh_calc_pub_key(const DhPrivKey* priv, DhPubKey* pub);
void dh_calc_shared_secret(const DhPrivKey* priv, const DhPubKey* pub, 
  uint8_t* ss);
#endif // DH_H__
