extern(C) void printf(const char *fmt, ...);
extern(C) void exit();

extern(C) int main(int argc, char **argv)
{
  printf("Hello, world!\n");
  exit();
  return 0;
}
