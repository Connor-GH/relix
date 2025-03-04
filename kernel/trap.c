#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include "drivers/mmu.h"
#include "drivers/lapic.h"
#include "drivers/ps2mouse.h"
#include "proc.h"
#include "x86.h"
#include "trap.h"
#include "traps.h"
#include "spinlock.h"
#include "ide.h"
#include "syscall.h"
#include "kbd.h"
#include "uart.h"
#include "console.h"
#include <stdint.h>
#include "time.h"
enum {
	PAGE_FAULT_PRESENT = 1 << 0,
	PAGE_FAULT_WRITE = 1 << 1,
	PAGE_FAULT_USER = 1 << 2,
	PAGE_FAULT_RESERVED_WRITE = 1 << 3,
	PAGE_FAULT_INSN_FETCH = 1 << 4,
	PAGE_FAULT_PROT_KEY_VIOLATION = 1 << 5,
	PAGE_FAULT_SHADOW_STACK = 1 << 6,
	PAGE_FAULT_SGX = 1 << 7,
};
// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uintptr_t vectors[]; // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
time_t ticks;
static void
decipher_page_fault_error_code(uint64_t error_code)
{
	uart_cprintf("This error code was caused for the following reasons: \n");
	if ((error_code & PAGE_FAULT_PRESENT) != PAGE_FAULT_PRESENT) {
		uart_cprintf("The present bit is not set.\n");
	}
	if (error_code & PAGE_FAULT_WRITE) {
		uart_cprintf("Caused by a write.\n");
	}
	if (error_code & PAGE_FAULT_USER) {
		uart_cprintf("CPL was set to 3 (user mode).\n");
	}
	if (error_code & PAGE_FAULT_RESERVED_WRITE) {
		uart_cprintf("A reserved bit was set to one.\n");
	}
	if (error_code & PAGE_FAULT_INSN_FETCH) {
		uart_cprintf("Caused by an instruction fetch.\n");
	}
	if (error_code & PAGE_FAULT_PROT_KEY_VIOLATION) {
		uart_cprintf("Caused by a Protection Key violation.\n");
	}
	if (error_code & PAGE_FAULT_SHADOW_STACK) {
		uart_cprintf("Caused by a shadow stack access.\n");
	}
	if (error_code & PAGE_FAULT_SGX) {
		uart_cprintf("Caused by an SGX violation.\n");
	}
}

void
trap(struct trapframe *tf)
{
	if (tf->trapno == T_SYSCALL) {
		if (myproc()->killed)
			exit(0);
		myproc()->tf = tf;
		syscall();
		if (myproc()->killed)
			exit(0);
		return;
	}

	switch (tf->trapno) {
	case T_IRQ0 + IRQ_TIMER:
		if (my_cpu_id() == 0) {
			acquire(&tickslock);
			ticks++;
			wakeup(&ticks);
			release(&tickslock);
		}
		lapiceoi();
		break;
	case T_IRQ0 + IRQ_IDE:
		ideintr();
		lapiceoi();
		break;
	case T_IRQ0 + IRQ_IDE + 1:
		// Bochs generates spurious IDE1 interrupts.
		break;
	case T_IRQ0 + IRQ_KBD:
		kbdintr();
		lapiceoi();
		break;
	case T_IRQ0 + IRQ_COM1:
		uartintr();
		lapiceoi();
		break;
	case T_IRQ0 + IRQ_PS2_MOUSE:
		ps2mouseintr();
		lapiceoi();
		break;
	case T_IRQ0 + 7:
	case T_IRQ0 + IRQ_SPURIOUS:
		uart_cprintf("cpu%d: spurious interrupt at %#lx:%#lx\n", my_cpu_id(), tf->cs,
						tf->eip);
		lapiceoi();
		break;
	case T_ILLOP:
		uart_cprintf("Illegal instruction\n");
		if ((tf->cs & 3) == DPL_USER) {
			kill(myproc()->killed, SIGILL);
		} else {
			panic("Illegal instruction in the kernel!");
		}
		break;
	case T_GPFLT:
		uart_cprintf("General protection fault\n");
		if ((tf->cs & 3) == DPL_USER) {
			kill(myproc()->killed, SIGSEGV);
			uart_cprintf("Process %s killed with SIGSEGV: sp=%#lx\n", myproc()->name, tf->esp);
			uart_cprintf("from cpu %d eip %lx (cr2=%#lx)\n",
							my_cpu_id(), tf->eip, rcr2());
		} else {
			uart_cprintf("BUG: General protection fault in the kernel!\n");
			uart_cprintf("from cpu %d eip %lx (cr2=%#lx)\n",
							my_cpu_id(), tf->eip, rcr2());
			// Skip over the faulting instruction and
			// hope for the best.
			tf->eip += 8;
		}
		break;
	case 7: {
		// We eagerly save FPU state so just clear the task switch bit.
		__asm__ __volatile__("clts");
		break;
	}
	// TODO handle pagefaults in a way that allows copy-on-write
	case T_PGFLT:
		uart_cprintf("Page fault at %#lx, ip=%#lx\n", rcr2(), tf->eip);
		decipher_page_fault_error_code(tf->err);
		if ((tf->cs & DPL_USER) == 0) {
			panic("trap");
		} else {
			uart_cprintf("Process %s killed with SIGSEGV: sp=%#lx\n", myproc()->name, tf->esp);
			uintptr_t pcs[10];
			getcallerpcs_with_bp(pcs, &tf->rbp, 10);
			for (int i = 0; i < 10; i++)
					uart_cprintf("%#lx ", pcs[i]);
				uart_cprintf("\n");
			kill(myproc()->pid, SIGSEGV);
		}
		break;
	case T_DIVIDE:
		uart_cprintf("%s[%d]: trap divide by zero error: %#lx\n", myproc()->name, myproc()->pid, tf->eip);
		kill(myproc()->pid, SIGFPE);
		break;
	case T_SIMDERR:
	case T_FPERR:
		uart_cprintf("%s[%d]: floating point error: %#lx\n", myproc()->name, myproc()->pid, tf->eip);
		kill(myproc()->pid, SIGFPE);
		break;
	default:
		if (myproc() == 0 || (tf->cs & 3) == 0) {
			// In kernel, it must be our mistake.
			uart_cprintf("unexpected trap %ld from cpu %d eip %lx (cr2=%#lx)\n",
							tf->trapno, my_cpu_id(), tf->eip, rcr2());
			panic("trap");
		}
		// In user space, assume process misbehaved.
		uart_cprintf("pid %d %s: trap %ld err %ld on cpu %d "
						"eip %#lx addr %#lx--kill proc\n",
						myproc()->pid, myproc()->name, tf->trapno, tf->err, my_cpu_id(),
						tf->eip, rcr2());
		myproc()->killed = 1;
	}

	// Force process exit if it has been killed and is in user space.
	// (If it is still executing in the kernel, let it keep running
	// until it gets to the regular system call return.)
	if (myproc() && myproc()->killed && (tf->cs & 3) == DPL_USER)
		exit(0);

	// Force process to give up CPU on clock tick.
	// If interrupts were on while locks held, would need to check nlock.
	if (myproc() && myproc()->state == RUNNING &&
			tf->trapno == T_IRQ0 + IRQ_TIMER)
		yield();

	// Check if the process has been killed since we yielded
	if (myproc() && myproc()->killed && (tf->cs & 3) == DPL_USER)
		exit(0);
}
