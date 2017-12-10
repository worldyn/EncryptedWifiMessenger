#ifndef DH_PARAMS_DH__
#define DH_PARAMS_DH__

#include "bigu.h"

#define DH_2048_A // 20 sec to calc pub key, 20 sec to calc shared secret.
//#define DH_1024_A // Does not work now since PRIV_LEN is not def here.


#ifdef DH_2048_A
/*
 * This is the 2048-bit MODP Group with 256-bit Prime Order Subgroup defined
 * in rfc5114 section 2.3.
 */

// They're 2048 bits (= 64 limbs = 256 bytes).
// So R = 2^2048, rlen = 64.
#define R_LEN 64
// 256 bit security -> 32 byte = 8 limb priv key.
#define PRIV_LEN  8
#endif // DH_2048_A

#ifdef DH_1024_A
#define R_LEN 32
#endif

extern Limb p[];

// Not from the doc but calculated based on p (and R).
// Chosen so that p*p_prim = -1 (mod R).
extern Limb p_prim[];

//extern int g[];
extern Limb gm[];

extern const MontgomeryCtx mctx;


#endif // DH_PARAMS_DH__
