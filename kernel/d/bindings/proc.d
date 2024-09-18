module proc;
@nogc nothrow:
extern(C): __gshared:
import param;
import mmu;
import x86;

enum SYSCALL_AMT = 29;
struct Groups {
	char* gr_name;
	char* gr_passwd;
	uint gr_gid;
	char** gr_mem;
}

struct Cred {
	uint uid;
	uint gid;
	Groups[MAXGROUPS] groups;
}

struct cpu {
	ubyte apicid; // Local APIC ID
	Context* scheduler; // swtch() here to enter scheduler
	taskstate ts; // Used by x86 to find stack for interrupt
	segdesc[NSEGS] gdt; // x86 global descriptor table
	/*volatile*/ uint started; // Has the CPU started?
	int ncli; // Depth of pushcli nesting.
	int intena; // Were interrupts enabled before pushcli?
	Proc* proc; // The process running on this cpu or null
}

extern cpu[NCPU] cpus;
extern int ncpu;

// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct Context {
	uint edi;
	uint esi;
	uint ebx;
	uint ebp;
	uint eip;
}

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE }
alias UNUSED = procstate.UNUSED;
alias EMBRYO = procstate.EMBRYO;
alias SLEEPING = procstate.SLEEPING;
alias RUNNABLE = procstate.RUNNABLE;
alias RUNNING = procstate.RUNNING;
alias ZOMBIE = procstate.ZOMBIE;

struct file;
struct inode;

// Per-process state
struct Proc {
	uint sz; // Size of process memory (bytes)
	int* pgdir; // Page table
	char* kstack; // Bottom of kernel stack for this process
	procstate state; // Process state
	int pid; // Process ID
	int status;
	Proc* parent; // Parent process
	trapframe* tf; // Trap frame for current syscall
	Context* context; // swtch() here to run process
	void* chan; // If non-zero, sleeping on chan
	int killed; // If non-zero, have been killed
	file*[NOFILE] ofile; // Open files
	inode* cwd; // Current directory
	Cred cred; // user's credentials for the process.
	char[16] name = 0; // Process name (debugging)
	char[SYSCALL_AMT] strace_mask_ptr = 0; // mask for tracing syscalls
}

