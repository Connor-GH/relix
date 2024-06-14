#include <umath.h>

unsigned int ui_pow(unsigned int base, unsigned int power) {
  if (power == 0) {
    return 1;
  }
  if (base == 0) {
    return 0;
  }

  unsigned int ret = 1;
  for (int i = 0; i < power; i++) {
    ret *= base;
  }
  return ret;
}
