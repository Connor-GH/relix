module memlayout;
@nogc nothrow:
extern(C): __gshared:
enum X64 = 0;
alias uintptr_t = uint;
static if (X64) {
enum KERNBASE = 0xFFFFFFFF80000000; // First kernel virtual address
enum DEVBASE = 0xFFFFFFFF40000000; // First device virtual address
} else {
enum KERNBASE = 0x80000000; // First kernel virtual address
enum DEVBASE = 0xFE000000; // First device virtual address
}
// Memory layout
enum kiB = (1024UL);
enum MiB = (1024 * kiB);
enum GiB = (1024 * MiB);

enum EXTMEM = 0x100000; // Start of extended memory
enum PHYSTOP = (256 * MiB - (32 * MiB)); //0xE000000 // Top physical memory
enum DEVSPACE = 0xFE000000; // Other devices are at high addresses

// Key addresses for address space layout (see kmap in vm.c for layout)
enum KERNLINK = (KERNBASE + EXTMEM); // Address where kernel is linked

enum string V2P(string a) = `((cast(uint_)(` ~ a ~ `)) - KERNBASE)`;
enum string P2V(string a) = `(cast(void*)((cast(char*)(` ~ a ~ `)) + KERNBASE))`;

enum string V2P_WO(string x) = `((x) - KERNBASE) // same as V2P, but without casts`;
enum string P2V_WO(string x) = `((x) + KERNBASE) // same as P2V, but without casts`;

// Key addresses for address space layout (see kmap in vm.c for layout)

version (__ASSEMBLER__) {} else {

pragma(inline, true) private uintptr_t v2p(void* a)
{
	return (cast(uintptr_t)(a)) - (cast(uintptr_t)KERNBASE);
}
pragma(inline, true) private void* p2v(uintptr_t a)
{
	return cast(void*)((a) + (cast(uintptr_t)KERNBASE));
}

}

enum string IO2V(string a) = `((cast(void*)(` ~ a ~ `)) + DEVBASE - DEVSPACE)`;
