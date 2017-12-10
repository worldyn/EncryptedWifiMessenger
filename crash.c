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

