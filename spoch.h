#ifndef SPOCH_H__
#define SPOCH_H__

void spoch(uint8_t msg[], uint32_t len, uint32_t digest_len,
  uint8_t out[digest_len]);

#endif // SPOCH_H__
