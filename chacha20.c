#include <stdint.h>
#include "chacha20.h"
#define ROUNDS 10

// Implemented via salsa20 speci. with chacha20 spec:
// https://cr.yp.to/snuffle/spec.pdf
// https://cr.yp.to/chacha/chacha-20080128.pdf

// https://blog.regehr.org/archives/1063
uint32 rot32(uint32 x, uint32 n)
{
  return (x<<n) | (x>>(32-n));
}

void quarterround(uint32 a, uint32 b, uint32 c, uint32 d) {
  a += b; d ^= a; d = rot32(d,16);
  c += d; b ^= c; b = rot32(b, 12);
  a += b; d ^= a; d = rot32(d,8);
  c += d; b ^= c; b = rot32(b,7);
}

void columnround(uint32 mat[]) {
  quarterround(mat[0], mat[4], mat[8],  mat[12]);
  quarterround(mat[1], mat[5], mat[9],  mat[13]);
  quarterround(mat[2], mat[6], mat[10],  mat[14]);
  quarterround(mat[3], mat[7], mat[11],  mat[15]);
}

void diagonalround(uint32 mat[]) {
  quarterround(mat[0], mat[5], mat[10],  mat[15]);
  quarterround(mat[1], mat[6], mat[11],  mat[12]);
  quarterround(mat[2], mat[7], mat[8],  mat[13]);
  quarterround(mat[3], mat[4], mat[9],  mat[14]);
}

void doubleround(uint32 mat[]) {
  columnround(mat);
  diagonalround(mat);
}

/*
Hash function
in and out is 64 bytes
*/
void chacha20_hash(const uint32 in[16], uint32 out[16]) {
  uint32 mat[16];
  
  // Copy in to mat
  for(byte i = 0; i < 16; ++i) {
    mat[i] = in[i]; 
  }
  
  // perform the double rounds
  for(byte i = 0; i < ROUNDS; ++i) {
    doubleround(mat);
  } 

  // add all elements in mat and in into out
  // TODO: make sure you think about little endian
  for(byte i = 0; i < 16; ++i) {
    out[i] = in[i] + mat[i];
  }
}

/*
Encryption function
the key k is 64 bytes,
the nonce v is 8 bytes,

*/
/*void chacha20_enc(const uint32 k[16]) {

}*/

/*
 Chacha20 encryption
*/
/*void encrypt(uint32 out[64], const uint32 in[16]) {
  uint32 mat[16];
  int i;

  // copy input to matrix
  for (i = 0; i < 16; ++i) { 
    mat[i] = in[i];
  }

  for (i = 0; i < ROUNDS; ++i) {
    doubleround(mat);
  }


}*/

void bytes_to_word(uint32* word, byte bytes[]) {
  *word = 0;
  *word |= (uint32)(bytes[0]) << 0;
  *word |= (uint32)(bytes[1]) << 8;
  *word |= (uint32)(bytes[2]) << 16;
  *word |= (uint32)(bytes[3]) << 24;
}

void chacha20_expand(uint32* matrix, const byte* nonce, const byte* key) {
  // Constants, byte 0-3
  // consts is "expand 32-byte k" in ascii to hex
  matrix[0] = 0x61707865;
  matrix[1] = 0x3320646E;
  matrix[2] = 0x79622D32; 
  matrix[3] = 0x6B206574;
  // Key, byte 4-11
  bytes_to_word(&matrix[4], key);
  bytes_to_word(&matrix[5], key + 4);
  bytes_to_word(&matrix[6], key + 8);
  bytes_to_word(&matrix[7], key + 12);
  bytes_to_word(&matrix[8], key + 16);
  bytes_to_word(&matrix[9], key + 20);
  bytes_to_word(&matrix[10], key + 24);
  bytes_to_word(&matrix[11], key + 28);
  // Counter, byte 12-13
  matrix[12] = 0; 
  matrix[13] = 0;
  // Nonce, bytes, 14-15
  bytes_to_word(&matrix[14], nonce); 
  bytes_to_word(&matrix[15], nonce + 4);
}

//void chacha20_hash_test() {
//  uint in[16] = {
//  
//  };
//}
