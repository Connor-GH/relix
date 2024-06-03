#include "../include/user.h"
#include "../../include/stdio.h"
#include "../../include/date.h"

int main(void) {
  struct rtcdate r;

  if (date(&r) != 0) {
    fprintf(stderr, "date failed\n");
  }
  fprintf(stdout, "%d-%d-%d %d:%d:%d\n", r.year, r.month, r.day, r.hour, r.minute, r.second);
  exit();
}
