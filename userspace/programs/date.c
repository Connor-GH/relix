#include <user.h>
#include <stdio.h>
#include <date.h>

int main(void) {
  struct rtcdate r;

  if (date(&r) != 0) {
    fprintf(stderr, "date failed\n");
  }
  printf("%04d-%02d-%02d %02d:%02d:%02d\n",
          r.year, r.month, r.day, r.hour, r.minute, r.second);
  exit();
}
