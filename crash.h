#ifndef CRASH_H__
#define CRASH_H__
#include "machine_defs.h"
void crash(char* msg0, char* msg1, char* msg2, char* msg3);
void print_buffer_start(u8 line, u8 bytes[], u8 len);
char nibble_to_hex(u8 nibble);
#endif //CRASH_H__
