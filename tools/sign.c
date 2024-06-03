#include <stdio.h> // fprintf
#include <stdlib.h> // EXIT_FAILURE
#include <fcntl.h> // O_RDWR
#include <unistd.h> // read, write, close, lseek
#include <assert.h> // assert
#include <string.h> // memset, memcpy

#define BUF_SIZE 512
int main(int argc, char **argv) {
  if (argc != 2)
    exit(EXIT_FAILURE);
  int fd = open(argv[1], O_RDWR , 0666);
  if (fd == -1) {
    perror("open");
    exit(EXIT_FAILURE);
  }
  char buf[BUF_SIZE];
  ssize_t n = read(fd, buf, sizeof(buf));
  if (n == -1) {
    perror("read");
    close(fd);
    exit(EXIT_FAILURE);
  }
  if (n > 510) {
    fprintf(stderr, "boot block too large: %zd bytes (max 510)\n", n);
    exit(EXIT_FAILURE);
  }
  fprintf(stderr, "boot block is %zd bytes (max 510)\n", n);


  char *padding = malloc(510 - (size_t)n);
  // put padding in padding variable
  memset(padding, '\0', 510 - (size_t)n);

  // append to buf with padding
  memcpy(buf + n, padding, 510 - (size_t)n);
  // padding is not used past here, clean it up
  free(padding);
  // append magic number at the end
  memcpy(buf + n + (510 - n), "\x55\xAA", 2);

  lseek(fd, 0, SEEK_SET);
  n = write(fd, buf, 512);
  if (n == -1) {
    perror("write");
    exit(EXIT_FAILURE);
  }
  // it's a problem if the file is not 512 bytes exactly
  assert(n == 512);
  n = read(fd, buf, 512);
  // also a problem if the bytes at the end are not the magic numbers
  assert(buf[510] == '\x55' && buf[511] == '\xAA');

  int c = close(fd);
  if (c == -1) {
    perror("close");
    exit(EXIT_FAILURE);
  }
  return 0;
}
