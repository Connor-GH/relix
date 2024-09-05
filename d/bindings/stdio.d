module stdio;
import bindings.stdarg;

alias FILE = int;
enum FILE __file_stdin = 0;
enum FILE __file_stdout = 1;
enum FILE __file_stderr = 2;

alias stdin = __file_stdin;
alias stdout = __file_stdout;
alias stderr = __file_stderr;

enum EOF = -1;
enum DIRSIZ = 254;
alias FILENAME_MAX = DIRSIZ;

extern(C) {
	void
	vfprintf(int, const char *, va_list *argp);
	void
	fprintf(int, const char *, ...);
	void
	printf(const char *, ...);
	void
	vsprintf(char *str, const char *fmt, va_list *argp);
	void
	sprintf(char *str, const char *fmt, ...);
	char *
	gets(char *, int max);
	int
	getc(FILE fd);
	@safe
	void
	perror(const char *s);
}
