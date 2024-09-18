module console;
extern(C) @nogc:
noreturn panic(const char *msg);
void cprintf(const char *fmt, ...);
