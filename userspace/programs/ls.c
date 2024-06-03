#include "../../include/stat.h"
#include "../include/user.h"
#include "../../kernel/include/fs.h"
#include "../../include/stdbool.h"
#include "../../include/stdlib.h"
#include "../../include/stdio.h"

char *
fmtname(char *path)
{
  static char buf[DIRSIZ + 1];
  char *p;

  // Find first character after last slash.
  for (p = path + strlen(path); p >= path && *p != '/'; p--)
	;
  p++;

  // Return blank-padded name.
  if (strlen(p) >= DIRSIZ)
	return p;
  memmove(buf, p, strlen(p));
  memset(buf + strlen(p), ' ', DIRSIZ - strlen(p));
  return buf;
}

// this ls(1) tries to follow __minimal__ POSIX stuff.
void
ls(char *path, bool lflag, bool iflag)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if ((fd = open(path, 0)) < 0) {
	fprintf(stderr, "ls: cannot open %s\n", path);
	return;
  }

  if (fstat(fd, &st) < 0) {
	fprintf(stderr, "ls: cannot stat %s\n", path);
	close(fd);
	return;
  }

  switch (st.type) {
  case T_FILE:
	  if (lflag) {
      // inodes
      if (iflag)
        fprintf(stdout, "% 9d ", st.st_ino);
      fprintf(stdout, "%d % 9d %s", st.type, st.st_size, fmtname(path));
      if (st.st_nlink > 1)
        fprintf(stdout, " -> %s", "TODO");
    } else {
      fprintf(stdout, "%s", fmtname(path));
    }
      fprintf(stdout, "\n");
	break;

  case T_DIR:
	if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
	  fprintf(stdout, "ls: path too long\n");
	  break;
	}
	strcpy(buf, path);
	p = buf + strlen(buf);
	*p++ = '/';
	while (read(fd, &de, sizeof(de)) == sizeof(de)) {
	  if (de.inum == 0)
		continue;
	  memmove(p, de.name, DIRSIZ);
	  p[DIRSIZ] = 0;
	  if (stat(buf, &st) < 0) {
		fprintf(stdout, "ls: cannot stat %s\n", buf);
		continue;
	  }
	  if (lflag) {
      // inodes
      if (iflag)
        fprintf(stdout, "% 9d ", st.st_ino);
      fprintf(stdout, "%d % 9d %s%s", st.type, st.st_size, fmtname(buf), ((st.st_mode & S_IEXEC) == S_IEXEC) ? "*" : "");
      if (st.st_nlink > 1)
        fprintf(stdout, " -> %s", "TODO");
      fprintf(stdout, "\n");
    } else {
      fprintf(stdout, "%s", fmtname(buf));
    }
	}
	break;
  default:
      break;
  }
  fprintf(stdout, "%s", lflag ? "" : "\n");
  close(fd);
}

int
main(int argc, char *argv[])
{
  int i;
  bool lflag = false;
  bool iflag = false;
  // index for filenames or directory names
  int arg_idx = 1;

  if (argc < 2) {
	  ls(".", lflag, iflag);
	  exit();
  }
  for (int j = 1; j < argc; j++) {
    if (argv[j][0] == '-' && argv[j][1] != '\0') {
      switch (argv[j][1]) {
      case 'l':
        lflag = true;
        break;
      case 'i':
        iflag = true;
        break;
      default:
        break;
      }
      arg_idx++;
    } else {
      break;
    }
  }
  if (arg_idx == argc) {
    ls(".", lflag, iflag);
    exit();
  }

  for (i = 1; i < argc; i++) {
    if (stat(argv[i], NULL) >= 0)
      ls(argv[i], lflag, iflag);
  }
  exit();
}
