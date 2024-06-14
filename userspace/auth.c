#include <user.h>
#include <auth.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

int usertouid(char *user) {
  int fd;
  if ((fd = open("/etc/passwd", 0)) < 0) {
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

char *
userto_allocated_passwd(char *user)
{
  int fd;
  char *put_in = malloc(512);
  if ((fd = open("/etc/passwd", 0)) < 0) {
    fprintf(stderr, "cannot open passwd_file\n");
    return NULL;
  }
  char buf[512] = {0};
  char info[512] = {0};
  int n;
  if ((n = read(fd, buf, sizeof buf)) <= 0) {
    fprintf(stderr, "cannot read from passwd_file\n");
    close(fd);
    return NULL;
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
    fprintf(stderr, "info: %c\n", info[j]);
    i++;
    j++;
  }
  info[j] = '\0';
  fprintf(stderr, "info: %s\n", info);
  close(fd);
  assert(info[0] != '\0');
  strcpy(put_in, info);
  assert(strcmp(put_in, info) == 0);
  return put_in;
}
