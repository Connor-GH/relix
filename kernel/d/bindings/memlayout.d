module memlayout;
@nogc nothrow:
extern(C): __gshared:
enum X64 = 1;
alias uintptr_t = ulong;
static if (X64) {
enum ulong KERNBASE = 0xFFFFFFFF80000000UL; // First kernel virtual address
enum ulong DEVBASE = 0xFFFFFFFF40000000UL; // First device virtual address
}
// Memory layout
enum kiB = (1024UL);
enum MiB = (1024 * kiB);
enum GiB = (1024 * MiB);

// Key addresses for address space layout (see kmap in vm.c for layout)

uintptr_t v2p(void* a)
{
	return cast(uintptr_t)(a - KERNBASE);
}
void* p2v(uintptr_t a)
{
	return cast(void*)(a + KERNBASE);
}
