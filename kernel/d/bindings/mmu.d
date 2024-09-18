module mmu;
extern(C) @nogc __gshared :
enum NSEGS = 6;

struct segdesc {
	ubyte[64] __unused;
}

struct taskstate {
	uint link; // Old ts selector
	uint esp0; // Stack pointers and segment selectors
	ushort ss0; //   after an increase in privilege level
	ushort padding1;
	uint *esp1;
	ushort ss1;
	ushort padding2;
	uint *esp2;
	ushort ss2;
	ushort padding3;
	void *cr3; // Page directory base
	uint *eip; // Saved state from last task switch
	uint eflags;
	uint eax; // More saved state (registers)
	uint ecx;
	uint edx;
	uint ebx;
	uint *esp;
	uint *ebp;
	uint esi;
	uint edi;
	ushort es; // Even more saved state (segment selectors)
	ushort padding4;
	ushort cs;
	ushort padding5;
	ushort ss;
	ushort padding6;
	ushort ds;
	ushort padding7;
	ushort fs;
	ushort padding8;
	ushort gs;
	ushort padding9;
	ushort ldt;
	ushort padding10;
	ushort t; // Trap on task switch
	ushort iomb; // I/O map base address
}
