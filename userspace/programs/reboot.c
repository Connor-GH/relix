#include <stdio.h>
#include <unistd.h>
#include <sys/reboot.h>

int
main(int argc, char **argv)
{
  if (argc == 1) {
  usage:
    fprintf(stderr, "usage: %s [-h|-p]\n", argv[0]);
    exit(0);
  }
  if (argv[1][0] == '-') {
    switch (argv[1][1]) {
    case 'h': reboot(RB_HALT); break;
    case 'p': reboot(RB_POWER_OFF); break;
    default: goto usage;
    }
  }
  goto usage;
}
