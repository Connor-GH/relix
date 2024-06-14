#include <stdio.h>
#include <user.h>


// int to char for numbers
static int ctoi_num(char c) {
  return (c - 0x30);
}

static unsigned long my_atoi(char *s) {
  unsigned long ret = 0;
  for (int i = 0; s[i] != '\0'; i++) {
    ret = ret * 10 + ctoi_num(s[i]);
  }
  return ret;
}

int main(int argc, char **argv) {
  if (argc == 1) {
    fprintf(stderr, "%s: [file]\n", argv[0]);
    exit(0);
  }
  int ret = sleep(my_atoi(argv[1]) * 100);
  while (ret != 0) {
    ret = sleep(ret);
  }
  exit(0);
}
