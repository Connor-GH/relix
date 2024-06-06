#include "../../include/reboot.h"
#include "../include/user.h"
#include "../../include/stdio.h"

int
main(int argc, char **argv)
{
  if (argc == 1) {
  usage:
    fprintf(stderr, "usage: %s [-h|-p]\n", argv[0]);
    exit();
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
