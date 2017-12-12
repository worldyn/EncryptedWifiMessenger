#include "crash.h"
#include <pic32mx.h>
#include <stdint.h>

void crash(char* msg0, char* msg1, char* msg2, char* msg3)
{
  if (msg0 != 0) {
    display_string(0, msg0);
  }
  if (msg1 != 0) {
    display_string(1, msg1);
  }
  if (msg2 != 0) {
    display_string(2, msg2);
  }
  if (msg3 != 0) {
    display_string(3, msg3);
  }

  display_update();

  uint8_t x = 1;
  uint32_t i;
  for (;;) {
    PORTE = ~x;
    x <<= 1;
    if (x == 0) {
      x = 1;
    }
    for (i = 0; i < 5e4; ++i);
  }
}

// Grabs the least sig 4 bits of passed byte and returns hex code.
char nibble_to_hex(u8 nibble)
{
  nibble &= 0x0F;
  if (nibble < 10) {
    return '0' + nibble;
  }

  return 'A' + (nibble-10);
}

// Prints up to the first 8 bytes of the passed array bytes to the first line.
void print_buffer_start(u8 line, u8 bytes[], u8 len) {
  len = (len == 0) || (len > 8) ? 8 : len;
  char str[17]; // 8 bytes = 16 hex digits + null term.
  str[16] = 0;

  u8 i;
  char c;
  for (i = 0; i < len; ++i) {
    c = nibble_to_hex(bytes[i] >> 4);
    str[i*2] = c;
    c = nibble_to_hex(bytes[i]);
    str[i*2 + 1] = c;
  }

  display_string(line, str);
}


