#ifndef BIGU_H__
#define BIGU_H__

// Borrowed from Olle
#include "machine_defs.h"

typedef u32 Limb;

//#define LIMB_BYTE_COUNT (sizeof(Limb))
// TODO: Size hardcoded like this so it can be used by preprocessor.
// Is this needed? Change back to above line?
#define LIMB_BYTE_COUNT 4
#define LIMB_BIT_COUNT (LIMB_BYTE_COUNT*8)

// Takes an array (not a pointer) of limbs as arg.
#define BIGU_LIMB_COUNT(bigu) (sizeof(bigu)/LIMB_BYTE_COUNT)


typedef struct
{
  const Limb* n;   // Of len nlen
  const Limb* np;  // Of len rlen
  u32 nlen;
  u32 rlen;

} MontgomeryCtx;

// Prints bigum in _reversed order_ meaning the most sig limbs are printed
// furthest to the right (useful for copy paste).
void bigu_print(const char* n, const Limb* a, u32 len);

#define LABEL_PRINT(bigu) bigu_print(#bigu, bigu, BIGU_LIMB_COUNT(bigu))
#define LABEL_PRINT_L(bigu,len) bigu_print(#bigu, bigu, len)


// Takes a bigu of limb count len and converts to an array of bytes.
// bigu: Array of 32 bit words in little endian (idx 0 is least sig).
// The output array of bytes will be big endian (idx 0 is most sig).
// Len of bytes is assumed to be equal len*LIMB_BYTE_COUNT.
void bigu_into_bytes(const Limb a[], u32 len, u8* bytes);

void bigu_from_bytes(const u8* bytes, u32 len, Limb a[]);


void bigu_set_zero(Limb a[], u32 len);

void bigu_set(const Limb s[], u32 len, Limb d[]);



// a and m are both of limb count nlen (of ctx).
// R = LIMB_MOD^r_len. Example: If Limb = u8 then R = 8^r_len.
//     Another example: If Limb is u32 and r_len=1 then R = 2^32.
// Converts a into montgomery form in mod n, base R, stores res in m.
void bigu_into_mont(const Limb a[], const MontgomeryCtx* ctx, Limb m[]);


// TODO: Make inline?
/*
void bigu_from_mont(const Limb t[], u32 tlen, const Limb n[], u32 nlen,
                                  u32 rlen, const Limb np[], Limb res[])
*/
void bigu_from_mont(const Limb t[], const MontgomeryCtx* ctx, Limb res[]);


// b and p are in montomery form.
// b, n and p are all of nlen length (limb count).
// p = b^2 (mod n)
void bigu_mont_pow(const Limb b[], const Limb e[], u32 elen,
                          const MontgomeryCtx* ctx, Limb p[]);



#endif // BIGU_H__



