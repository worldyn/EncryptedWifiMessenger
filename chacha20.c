#include <stdint.h>

typedef uint32_t uint32;
#define ROUNDS 10

void quarter_round(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
  a += b; d ^= a; d <<<= 16;
  c += d; b ^= c; b <<<= 12;
  a += b; d ^= a; d <<<= 8;
  c += d; b ^= c; b <<<= 7;
}

void chacha20_encrypt(unsigned char out[64], const unsigned int in[16]) {
  unsigned int mat[16];
  int i;

  // copy input to matrix
  for (i = 0; i < 16; ++i) { mat[i] = in[i]; }

  for (i = 0; i < ROUNDS; ++i) {
    quarter_round(mat[0], mat[4], mat[8],  mat[12]);
    quarter_round(mat[1], mat[5], mat[9],  mat[13]);
    quarter_round(mat[2], mat[6], mat[10],  mat[14]);
    quarter_round(mat[3], mat[7], mat[11],  mat[15]);

    quarter_round(mat[0], mat[5], mat[10],  mat[15]);
    quarter_round(mat[1], mat[6], mat[11],  mat[12]);
    quarter_round(mat[2], mat[7], mat[8],  mat[13]);
    quarter_round(mat[3], mat[4], mat[9],  mat[14]);
  }


}
