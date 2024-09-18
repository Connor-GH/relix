module kernel_string;
@nogc nothrow:
extern(C): __gshared:
int memcmp(const(void)*, const(void)*, uint);
deprecated("Removed in POSIX.1-2008") int bcmp(const(void)*, const(void)*, uint);
void* memcpy(void*, const(void)*, uint);
void* memmove(void*, const(void)*, uint);
void* memset(void*, int, uint);
char* safestrcpy(char*, const(char)*, int);
char* strlcpy_nostrlen(char* dst, const(char)* src, int dst_len, int src_len);
uint strlen(const(char)*);
int strncmp(const(char)*, const(char)*, uint);
char* strncpy(char*, const(char)*, int);
char* strcat(char* dst, const(char)* src);
