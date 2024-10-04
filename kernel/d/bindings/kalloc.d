module kalloc;
extern(C):
char *kpage_alloc();
void kpage_free(char *ptr);
void *kmalloc(size_t);
void kfree(void *ptr);
