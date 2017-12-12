#ifndef MACHINE_DEFS_H
#define MACHINE_DEFS_H
// Borrowed from Olle
// Put all machine dependent defintions in this file. The only difference
// between code working on x64 linux and on ChipKit Uno32 should be the
// definitions in this file.

#define X64 1
#define MIPS32 2

#define PLATFORM MIPS32
//#define PLATFORM X64

#ifndef PLATFORM
#error "You must define a valid platform in machine_defs.h"
#elif PLATFORM == X64


// Linux x64:
#include<stdint.h>
typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef unsigned char bool;
#define TRUE 1
#define FALSE 0
#define LITTLE_ENDIAN

// No eqv on mips32. Only used by LIMB_MUL which
// will be impl differently on mips.
typedef uint64_t u64;


#elif PLATFORM == MIPS32


#include<stdint.h>
typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef unsigned char bool;
#define TRUE 1
#define FALSE 0
// TODO: Is it in little endian?
#define LITTLE_ENDIAN


#else
#error "You must define a valid platform in machine_defs.h"
#endif





#endif // MACHINE_DEFS_H
