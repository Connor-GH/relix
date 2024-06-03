#pragma once
#define T_DIR 1 // Directory
#define T_FILE 2 // File
#define T_DEV 3 // Device

#ifndef USE_HOST_STAT
#include "types.h"
struct stat {
  short type; // Type of file
  int st_dev; // File system's disk device
  uint st_ino; // Inode number
  short st_nlink; // Number of links to file
  uint st_size; // Size of file in bytes
#define S_IEXEC 0100
#define S_IREAD 0400
#define S_IWRITE 0200
#define S_IFREG 0100000
  uint st_mode;
  uint st_uid;
  uint st_gid;
};
#endif
