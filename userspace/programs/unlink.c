#include "../../include/stdio.h"
#include "../include/user.h"

int main(int argc, char **argv) {
  if (argc == 1) {
    fprintf(stderr, "%s: [file]\n", argv[0]);
    exit();
  }
  int ret = unlink(argv[1]);
  if (ret == -1) {
    fprintf(stderr, "%s: failure to unlink %s\n", argv[0], argv[1]);
    exit(); // TODO exit needs to return a value
  }
  exit();
}
