// Borrowed from Olle
#include <stdio.h>
#include "bigu.h"

#define MIN(x,y) ((x)<(y) ? (x) : (y))
#define MAX(x,y) ((x)>(y) ? (x) : (y))



// A BigU is an array of limbs in small endian representation.
// Ie, limb and index 0 is the least sig limb.


// Evals to 1 if bit with index i is 1 in BigU a. 0 otherwise 
#define BIGU_BIT_IDX(a, i) \
  (\
    (\
      (a)[(i)/LIMB_BIT_COUNT] & /*Get the correct limb*/\
      (1 << ((i)%LIMB_BIT_COUNT)) /*Correct bit*/\
    ) >> ((i)%LIMB_BIT_COUNT) /*Shift bit down to pos 0*/\
  )

// Adds limbs a and b (and u8 cin), stores sum in s.
// Sets cout to 1 if overflow, 0 otherwise.
// cin and cout can be the same variable.
// s, a and b could all be the same variable.
// a, b, c : Limb
// cin, cout : u8
#define LIMB_ADD(cin, a, b, s, cout) \
  do {\
    register Limb limbadd__s = (a) + (b) + (cin);\
    (cout) = limbadd__s < MIN((a),(b)) ? 1 : 0;\
    (s) = limbadd__s;\
  } while(0);

// Multiplies a and b.
// Sets pl = lower half (limb) of the product.
// Sets pu = upper half (limb) of the product.
#if PLATFORM == X64
#define LIMB_MUL(a, b, pl, pu) \
  do {\
    u64 prod = ((u64)(a)) * ((u64)(b));\
    (pl) = prod;\
    (pu) = prod >> LIMB_BIT_COUNT;\
  } while (0);
#elif PLATFORM == MIPS32
// TODO: Maybe inline asm for better performance?
#define LIMB_MUL(a, b, pl, pu)\
  do {\
    (pl) = mul_low(a, b);\
    (pu) = mul_upper(a, b);\
  } while (0);
#endif // PLATFORM == MIPS32 / X64
      

#define BIGU_LEN_CHECK(a, alen, b, blen, small, small_len, big, big_len) \
  do {\
    if ((alen) < (blen)) {\
      (small) = (a);\
      (big) = (b);\
      (small_len) = (alen);\
      (big_len) = (blen);\
    }\
    else {\
      (small) = (b);\
      (big) = (a);\
      (small_len) = (blen);\
      (big_len) = (alen);\
    }\
  } while(0);


#if PLATFORM == X64
// Prints bigum in _reversed order_ meaning the most sig limbs are printed
// furthest to the right (useful for copy paste).
void bigu_print(const char* n, const Limb* a, u32 len)
{
  printf("[");
  // A was name passed.
  if (! (n == 0 || *n == '\0') ) {
    printf("%s = ", n);
  }
  printf("BigU of %u limbs:", len);

  u32 i;
  for (i = len; i-- > 0 ;) {
    printf(" %08x", a[i]);
  }

  printf("]\n");
}
#endif // PLATFORM == X64

// Takes a bigu of limb count len and converts to an array of bytes.
// bigu: Array of 32 bit words in little endian (idx 0 is least sig).
// The output array of bytes will be big endian (idx 0 is most sig).
// Len of bytes is assumed to be equal len*LIMB_BYTE_COUNT.
void bigu_into_bytes(const Limb a[], u32 len, u8* bytes)
{
  register u32 w;
  u32 i, j;
  // i goes len-1..0 (most sig to least sig in a)
  // j goes 0..LIMB_BYTE_COUNT*len-1 (most sig to least sig in bytes)
  for (j = 0, i = len; i-- > 0; j+=LIMB_BYTE_COUNT) {
    w = a[i];
    // TODO: This assumes LIMB_BYTE_COUNT is 4. Is this ok?
#if LIMB_BYTE_COUNT == 4
    bytes[j+3] = (u8) w;
    w = w >> 8;
    bytes[j+2] = (u8) w;
    w = w >> 8;
    bytes[j+1] = (u8) w;
    w = w >> 8;
    bytes[j+0] = (u8) w;
#else
#error "LIMB_BYTE_COUNT must be 4"
#endif
  }
}

void bigu_from_bytes(const u8* bytes, u32 len, Limb a[])
{
  register u32 w;
  u32 i, j;
  // i goes len-1..0 (most sig to least sig in a)
  // j goes 0..LIMB_BYTE_COUNT*len-1 (most sig to least sig in bytes)
  for (j = 0, i = len; i-- > 0; j+=LIMB_BYTE_COUNT) {
    w = 0;
    // TODO: This assumes LIMB_BYTE_COUNT is 4. Is this ok?
#if LIMB_BYTE_COUNT == 4
    w |= bytes[j+0];
    w = w << 8;
    w |= bytes[j+1];
    w = w << 8;
    w |= bytes[j+2];
    w = w << 8;
    w |= bytes[j+3];
#else
#error "LIMB_BYTE_COUNT must be 4"
#endif
    a[i] = w;
  }
}


void bigu_set_zero(Limb a[], u32 len)
{
  for (; len-- > 0 ; ++a) {
    *a = 0;
  }
}

void bigu_set(const Limb s[], u32 len, Limb d[])
{
  /*
  for (; len-- > 0; ++s, ++d) {
    *d = *s;
  }
  */
  u32 i;
  for (i = len; i-- > 0;) {
    d[i] = s[i];
  }
}

// Bitwise flip of each bit.
void bigu_not(const Limb a[], u32 len, Limb d[])
{
  for (; len-- > 0 ; ++a, ++d) {
    *d = ~(*a);
  }
}

// Assumes alen >= blen.
// s = a + b. return 0 if no carry (no overflow), 1 if carry (overflow).
// limb count of s = MAX(alen, blen)
// carry_in should be 0 or 1.
// Can be done inplace (it's ok that s == a, s == b or s == a == b).
u8 bigu_addc_ord(u8 carry_in, const Limb a[], u32 alen, const Limb b[],
                                                     u32 blen, Limb s[])
{
  u32 i;
  u8 carry = carry_in;

  for (i = 0; i < blen; ++i) {
    LIMB_ADD(carry, b[i], a[i], s[i], carry);
  }

  for (; i < alen; ++i) {
    LIMB_ADD(carry, 0, a[i], s[i], carry);
  }

  return carry;
}

// TODO: Make inline.
u8 bigu_addc(u8 carry_in, const Limb a[], u32 alen, const Limb b[], u32 blen,
                                                                     Limb s[])
{
  if (alen >= blen) {
      return bigu_addc_ord(carry_in, a, alen, b, blen, s);
  }

  return bigu_addc_ord(carry_in, b, blen, a, alen, s);
}

void bigu_subtract(const Limb a[], u32 alen, const Limb b[], const u32 blen,
                                                                    Limb s[])
{
  Limb inv[blen];
  bigu_not(b, blen, inv);
  bigu_addc(1, a, alen, inv, blen, s);
}

// Same as bigu_addc but with default carry_in = 0.
// TODO: This should go in a header: add inline
/*inline*/ u8 bigu_add(const Limb a[], u32 alen, const Limb b[], u32 blen,
                                                                  Limb s[])
{
  return bigu_addc(0, a, alen, b, blen, s);
}


// Multiplies BigUs a and b and stores the product in p.
// p has limb count alen + blen
// Uses long multiplication.
// Can _not_ be done in place: p != a and p != b.
// a == b is ok.
// TODO: Use faster algo?
void bigu_mul(const Limb a[], u32 alen, const Limb b[], u32 blen, Limb p[])
{
  Limb pl, pu, carry;
  u32 ai, bi;
  u8 add_c;

  // Init p = 0.
  for (ai = 0 ; ai < alen+blen ; ++ai) {
    p[ai] = 0;
  }

  for (ai = 0 ; ai < alen ; ++ai) {
    carry = 0;
    for (bi = 0 ; bi < blen ; ++bi) {
      LIMB_MUL(a[ai], b[bi], pl, pu);

      LIMB_ADD(0, carry, pl, pl, add_c);
      pu += add_c;

      LIMB_ADD(0, p[ai+bi], pl, pl, add_c);
      pu += add_c;

      carry = pu;
      p[ai+bi] = pl;
    }
    p[ai + blen] += carry;
  }
}


// Inplace left shift a offset bits.
// Ie, shift towards higher sig.
void bigu_lshift(Limb a[], u32 len, u32 offset)
{
  u32 bit_sh = offset % LIMB_BIT_COUNT;
  u32 limb_sh = offset / LIMB_BIT_COUNT;

  u32 i;

  // Since lowest sig limbs are "to the left" this looks like a right shift.
  // Left shift = Shift towards higher sig = multiply by 2^offset.

  // Start by offsetting whole limbs.
  for (i = 0; i < len-limb_sh; ++i) {
    a[i+limb_sh] = a[i];
  }

  // Set the limbs_sh lowest to zero.
  for (i = 0; i < limb_sh; ++i) {
    a[i] = 0;
  }

  // Shift each Limbs by remain bit offset.
  Limb overflow = 0, cpy;
  for (i = 0; i < len; ++i) {
    cpy = a[i];
    a[i] = (a[i] << bit_sh) | overflow;
    overflow = cpy >> (LIMB_BIT_COUNT-bit_sh);
  }
}

// Returns  0 if big == small.
// Returns -1 if big < small.
// Returns  1 if big > small.
// Assumes that blen >= slen.
i8 bigu_comp_ord(const Limb big[], u32 blen, const Limb small[], u32 slen)
{
  u32 i;

  // Check if all "over bytes" in big are all 0.
  for (i = slen; i < blen; ++i) {
    if (big[i]) {
      return 1;
    }
  }

  // Most sig to least sig limbs.
  for (i = slen; i-- > 0;) {
    // big > small
    if (big[i] > small[i]) {
      return 1;
    }

    // a < b.
    if (big[i] < small[i]) {
      return -1;
    }
  }

  // They're equal.
  return 0;
}

// TODO: Make inline
i8 bigu_comp(const Limb a[], u32 alen, const Limb b[], u32 blen)
{
  if (alen >= blen) {
    return bigu_comp_ord(a, alen, b, blen);
  }

  // alen < blen
  return bigu_comp_ord(b, blen, a, alen);
}



// Uses ol boring long divsion. TODO: Faster algo?
// r is assumed to be of len den_len.
void bigu_mod(const Limb num[], u32 num_len, const Limb den[], u32 den_len,
                                                                   Limb r[])
{
  u32 i;
  bool lshift_overflow;

  // Bitwise flipped version of den. Used to subtract den from r.
  Limb inv_den[den_len];

  // Set up inv_den.
  bigu_not(den, den_len, inv_den);

  // Init r to 0.
  for (i = 0 ; i < den_len ; ++i) {
    r[i] = 0;
  }

  // Go through bits of num, most sig to least sig.
  for (i = num_len*LIMB_BIT_COUNT; i-- > 0;) {
    // overflow = true if most sig bit is 1.
    lshift_overflow = r[den_len-1] >> (LIMB_BIT_COUNT-1);
    bigu_lshift(r, den_len, 1);

    // least sig bit of r = bit i of num.
    if (BIGU_BIT_IDX(num, i)) {
      r[0] |= 1;
    }

    if (lshift_overflow || bigu_comp(r, den_len, den, den_len) >= 0) {
      // r = r - den
      bigu_addc(1, r, den_len, inv_den, den_len, r);
    }
  }
}


// a and m are both of limb count nlen (of ctx).
// R = LIMB_MOD^r_len. Example: If Limb = u8 then R = 8^r_len.
//     Another example: If Limb is u32 and r_len=1 then R = 2^32.
// Converts a into montgomery form in mod n, base R, stores res in m.
void bigu_into_mont(const Limb a[], const MontgomeryCtx* ctx, Limb m[])
{
  register u32 i;
  register u32 rlen = ctx->rlen;
  register u32 nlen = ctx->nlen;

  Limb p[nlen + rlen];
  const u32 plen = BIGU_LIMB_COUNT(p);

  // p = a*R.
  for (i = 0; i < nlen; ++i) {
    p[i + rlen] = a[i];
  }
  // Set the lowest limbs of p to 0.
  for (i = 0; i < rlen; ++i) {
    p[i] = 0;
  }

  // m = p%n
  bigu_mod(p, plen, ctx->n, nlen, m);
}

// Montgomery Reduction.
// R = LIMB_SIZE^rlen, see bigu_into_mont for examples.
// length of np same as R.
// length of res same as n.
// TODO: Use specialized verion for bigint from wiki?
/*
void bigu_redc(const Limb t[], u32 tlen, const Limb n[], u32 nlen,
                             u32 rlen, const Limb np[], Limb res[])
*/
void bigu_redc(const Limb t[], u32 tlen, const MontgomeryCtx* ctx, Limb res[])
{
  register u32 i;
  register u32 nlen = ctx->nlen;
  register u32 rlen = ctx->rlen;

  Limb x[rlen];
  Limb m[rlen];
  Limb y[nlen + rlen];
  Limb z[MAX(tlen,(nlen+rlen))+1];
  Limb v[BIGU_LIMB_COUNT(z) - rlen];
  
  // x = t % R, ie: grab rlen lowest limbs of t.
  for (i = 0; i < rlen; ++i) {
    x[i] = t[i];
  }

  // y = x*np.
  bigu_mul(x, rlen, ctx->np, nlen, y);

  // m = y % R
  for (i = 0; i < rlen; ++i) {
    m[i] = y[i];
  }

  // y = m*n
  bigu_mul(m, rlen, ctx->n, nlen, y);

  // z = t+y.
  u8 c = bigu_add(t, tlen, y, nlen+rlen, z);
  z[BIGU_LIMB_COUNT(z) - 1] = c; // Set most sig limb.

  // v = z / R.
  for (i = rlen; i < BIGU_LIMB_COUNT(z); ++i) {
    v[i-rlen] = z[i];
  }

  if (bigu_comp(v, BIGU_LIMB_COUNT(v), ctx->n, nlen) >= 0) {
    // res = v - N.
    bigu_subtract(v, BIGU_LIMB_COUNT(v), ctx->n, nlen, res);
  }
  else {
    // res = v.
    for (i = 0; i < nlen; ++i) {
      res[i] = v[i];
    }
  }

}

// TODO: Make inline
/*
void bigu_from_mont(const Limb t[], u32 tlen, const Limb n[], u32 nlen,
                                  u32 rlen, const Limb np[], Limb res[])
*/
void bigu_from_mont(const Limb t[], const MontgomeryCtx* ctx, Limb res[])
{
  bigu_redc(t, ctx->nlen, ctx, res);
}

// a, b and p are all nlen.
void bigu_mont_mul(const Limb a[], const Limb b[], const MontgomeryCtx* ctx, Limb p[])
{
  register u32 i;
  register const u32 nlen = ctx->nlen;
  register const u32 rlen = ctx->rlen;

  Limb prod[nlen+rlen];

  bigu_mul(a, nlen, b, nlen, prod);
  bigu_redc(prod, BIGU_LIMB_COUNT(prod), ctx, p);
}


// b and p are in montomery form.
// b, n and p are all of nlen length (limb count).
// p = b^2 (mod n)
void bigu_mont_pow(const Limb b[], const Limb e[], u32 elen,
                          const MontgomeryCtx* ctx, Limb p[])
{
  register u32 i;
  register const u32 nlen = ctx->nlen;

  Limb x[nlen];

  // TODO: Start by finding most sig non zero word. Search bitwise from there.
  // Find first (most sig) bit that is not 0.
  for (i = elen*LIMB_BIT_COUNT; i-- > 0;) {
    if (BIGU_BIT_IDX(e, i)) {
      break;
    }
  }

  // If e == 0: return 1.
  // TODO: Can't return actual 1, must return montgomery form of 1 (R mod N).
  if (i >= elen*LIMB_BIT_COUNT) { // If i went below 0.
    bigu_set_zero(p, nlen);
    p[0] = 1;
    //printf("e=0, UNSUPPORTED!\n");
    //fprintf(stderr, "e=0, UNSUPPORTED!\n");
    *((int*)0) = 12;
    return;
  }

  // e is at least 1 so start with p = b.
  bigu_set(b, nlen, p);

  for (;i-- > 0;) {
    // New digit in e, start by squaring p.
    bigu_mont_mul(p, p, ctx, p);

    // If this digit is 1: multiply by base.
    if (BIGU_BIT_IDX(e, i)) {
      bigu_mont_mul(p, b, ctx, p);
    }
  }
}





