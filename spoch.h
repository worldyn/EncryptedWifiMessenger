#ifndef SPOCH_H__
#define SPOCH_H__
#include "machine_defs.h"
// Hashes the msg with the spoch hash
void spoch(const uint8_t msg[], uint32_t msg_len, uint32_t dgst_len,
  uint8_t dgst_out[]);
#endif // SPOCH_H__
