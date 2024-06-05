#include "../include/user.h"
#include "../../include/stdio.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "chmod [octal] file\n");
    exit();
  }
  int mode = atoi_base(argv[1], 8);
  if (mode == 0) {
    fprintf(stderr, "Supply an octal node: e.g. 0677.\n");
    exit();
  }
  int ret = chmod(argv[2], mode);
  (void)ret;
  exit();
}
