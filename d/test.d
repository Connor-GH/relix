extern(C) void printf(const char *fmt, ...);
extern(C) noreturn exit(int);

extern(C) int main(int argc, char **argv)
{
  printf("Hello, world!\n");
  exit(0);
}
