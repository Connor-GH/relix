module string_;
@nogc nothrow:
extern(C): __gshared:
int memcmp(const(void)*, const(void)*, uint);
void* memcpy(void*, const(void)*, uint);
void* memmove(void*, const(void)*, uint);
void* memset(void*, int, uint);
uint strlen(const(char)*);
int strncmp(const(char)*, const(char)*, uint);
char* strncpy(char*, const(char)*, int);
char* strcat(char* dst, const(char)* src);
