#pragma once
#include <sys/types.h>

// POSIX definition of passwd
struct passwd {
  char *pw_name;
  uid_t pw_uid;
  gid_t pw_gid;
  char *pw_dir;
  char *pw_shell;
};
