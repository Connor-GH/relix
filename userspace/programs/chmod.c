#include <user.h>
#include <stdio.h>

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "chmod [octal] file\n");
    exit(0);
  }
  int mode = atoi_base(argv[1], 8);
  if (mode == 0) {
    fprintf(stderr, "Supply an octal node: e.g. 0677.\n");
    exit(0);
  }
  int ret = chmod(argv[2], mode);
  (void)ret;
  exit(0);
}
