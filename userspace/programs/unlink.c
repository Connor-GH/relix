#include <stdio.h>
#include <user.h>

int main(int argc, char **argv) {
  if (argc == 1) {
    fprintf(stderr, "%s: [file]\n", argv[0]);
    exit(0);
  }
  int ret = unlink(argv[1]);
  if (ret == -1) {
    fprintf(stderr, "%s: failure to unlink %s\n", argv[0], argv[1]);
    exit(0); // TODO exit needs to return a value
  }
  exit(0);
}
