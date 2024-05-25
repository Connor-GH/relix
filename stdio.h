#ifndef __STDIO_H
#define __STDIO_H
struct _IO_FILE {
  /* TODO */
};

typedef int FILE;

#define __file_stdin 0
#define __file_stdout 1
#define __file_stderr 2

/* TODO turn this into FILE * when we get proper stdio */
#define stdin ((FILE)__file_stdin)
#define stdout ((FILE)__file_stdout)
#define stderr ((FILE)__file_stderr)

#endif /* __STDIO_H */
