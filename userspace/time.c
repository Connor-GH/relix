#include "../include/time.h"
#include "../include/stdlib.h"
#include "../include/types.h"

uint time(uint *tloc) {
  uint val = 1; // fix later
  if (tloc != NULL)
    tloc = &val;
  return val;
}
