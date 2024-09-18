module x86;
@nogc nothrow:
extern(C): __gshared:
struct trapframe {
	// registers as pushed by pusha
	uint edi;
	uint esi;
	uint ebp;
	uint oesp; // useless & ignored
	uint ebx;
	uint edx;
	uint ecx;
	uint eax;

	// rest of trap frame
	ushort gs;
	ushort padding1;
	ushort fs;
	ushort padding2;
	ushort es;
	ushort padding3;
	ushort ds;
	ushort padding4;
	uint trapno;

	// below here defined by x86 hardware
	uint err;
	uint eip;
	ushort cs;
	ushort padding5;
	uint eflags;

	// below here only when crossing rings, such as from user to kernel
	uint esp;
	ushort ss;
	ushort padding6;
}
