#include "include/user.h"
#include "include/auth.h"
#include "../include/stdio.h"
#include "../include/stdlib.h"

int usertouid(char *user) {
  int fd;
  if ((fd = open("passwd_file", 0)) < 0) {
    fprintf(stderr, "cannot open passwd_file\n");
    return -1;
  }
  char buf[512];
  int n;
  if ((n = read(fd, buf, sizeof buf)) <= 0) {
    fprintf(stderr, "cannot read from passwd_file\n");
    close(fd);
    return -1;
  }
  int i = 0;
  int j = 0;
  char info[512];
  while (buf[i] != ':' && buf[i] != '\0') {
    info[j++] = buf[i++];
  }
  if (strcmp(user, info) == 0) {
    i++;
    j = 0;
    while (buf[i] != ':' && buf[i] != '\0') {
      info[j++] = buf[i++];
    }
    close(fd);
    return atoi(info);
  }
  close(fd);
  return -1;
}

void
usertopasswd(char *user, char *put_in)
{
  int fd;
  if ((fd = open("passwd_file", 0)) < 0) {
    fprintf(stderr, "cannot open passwd_file\n");
    put_in = NULL;
    return;
  }
  char buf[512];
  char info[512];
  int n;
  if ((n = read(fd, buf, sizeof buf)) <= 0) {
    fprintf(stderr, "cannot read from passwd_file\n");
    put_in = NULL;
    close(fd);
    return;
  }
  int i = 0;
  int j = 0;
  // username
  while (buf[i] != ':' && buf[i] != '\0') {
    i++;
  }
  i++;
  // uid
  while (buf[i] != ':' && buf[i] != '\0') {
    i++;
  }
  i++;
  // pass
  while (buf[i] != ':' && buf[i] != '\0' && buf[i] != '\n') {
    info[j] = buf[i];
    i++;
    j++;
  }
  close(fd);
  strcpy(put_in, info);
}
