#include "../include/user.h"
#include "../../include/stdlib.h"
#include "../../include/stdio.h"
#include "../../include/assert.h"
static void nulltest(void) {
  volatile int *ptr = NULL;
  *ptr = 6;
  assert(*ptr == 6);
}
int main(void) {
  nulltest();
  fprintf(stderr, "Tests passed! (This is not a good thing!)\n");
  exit();
}
