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
	uart_printf("This error code was caused for the following reasons: \n");
	if ((error_code & PAGE_FAULT_PRESENT) != PAGE_FAULT_PRESENT) {
		uart_printf("The present bit is not set.\n");
	}
	if (error_code & PAGE_FAULT_WRITE) {
		uart_printf("Caused by a write.\n");
	}
	if (error_code & PAGE_FAULT_USER) {
		uart_printf("CPL was set to 3 (user mode).\n");
	}
	if (error_code & PAGE_FAULT_RESERVED_WRITE) {
		uart_printf("A reserved bit was set to one.\n");
	}
	if (error_code & PAGE_FAULT_INSN_FETCH) {
		uart_printf("Caused by an instruction fetch.\n");
	}
	if (error_code & PAGE_FAULT_PROT_KEY_VIOLATION) {
		uart_printf("Caused by a Protection Key violation.\n");
	}
	if (error_code & PAGE_FAULT_SHADOW_STACK) {
		uart_printf("Caused by a shadow stack access.\n");
	}
	if (error_code & PAGE_FAULT_SGX) {
		uart_printf("Caused by an SGX violation.\n");
	}
}
static void
decipher_error_code_nonpagefault(uint64_t error_code)
{
	uart_printf("This error code was caused for the following reasons: \n");
	if (error_code % 2 != 0) {
		uart_printf("- happened due to external hardware (outside of processor)\n");
	}
	uart_printf("- selector index references a descriptor in the ");

	// 0b11 in binary
	switch ((error_code >> 1) & 3) {
	case 0:
		uart_printf("GDT (0b00)\n");
		break;
	case 1:
		uart_printf("IDT (0b01)\n");
		break;
	case 2:
		uart_printf("LDT (0b10)\n");
		break;
	case 3:
		uart_printf("IDT (0b11)\n");
		break;
	default:
		break;
	}
	uart_printf("In the index: %lu\n", (error_code & 0x0000FFFF) >> 3);
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
	case T_IRQ0 + IRQ_SATA:
		panic("SATA IRQ should not be reached (currently)");
		break;
	case T_IRQ0 + IRQ_PS2_MOUSE:
		ps2mouseintr();
		lapiceoi();
		break;
	case T_IRQ0 + 7:
	case T_IRQ0 + IRQ_SPURIOUS:
		uart_printf("cpu%d: spurious interrupt at %#lx:%#lx\n", my_cpu_id(), tf->cs,
						tf->rip);
		lapiceoi();
		break;
	case T_ILLOP:
		uart_printf("Illegal instruction\n");
		if ((tf->cs & 3) == DPL_USER) {
			kill(myproc()->killed, SIGILL);
		} else {
			panic("Illegal instruction in the kernel!");
		}
		break;
	case T_GPFLT:
		uart_printf("General protection fault\n");
		if ((tf->cs & 3) == DPL_USER) {
			kill(myproc()->killed, SIGSEGV);
			uart_printf("Process %s killed with SIGSEGV: sp=%#lx\n", myproc()->name, tf->rsp);
			uart_printf("from cpu %d rip %lx (cr2=%#lx)\n",
							my_cpu_id(), tf->rip, rcr2());
		} else {
			uart_printf("BUG: General protection fault in the kernel!\n");
			uart_printf("from cpu %d rip %lx (cr2=%#lx)\n",
							my_cpu_id(), tf->rip, rcr2());
			panic("kernel general protection fault");
		}
		break;
	case 7: {
		// We eagerly save FPU state so just clear the task switch bit.
		__asm__ __volatile__("clts");
		break;
	}
	// TODO handle pagefaults in a way that allows copy-on-write
	case T_PGFLT:
		uart_printf("Page fault at %#lx, ip=%#lx\n", rcr2(), tf->rip);
		decipher_page_fault_error_code(tf->err);
		if ((tf->cs & DPL_USER) == 0) {
			panic("trap");
		} else {
			uart_printf("Process %s killed with SIGSEGV: sp=%#lx\n", myproc()->name, tf->rsp);
			uintptr_t pcs[10];
			getcallerpcs_with_bp(pcs, &tf->rbp, 10);
			for (int i = 0; i < 10; i++)
					uart_printf("%#lx ", pcs[i]);
				uart_printf("\n");
			kill(myproc()->pid, SIGSEGV);
		}
		break;
	case T_DIVIDE:
		uart_printf("%s[%d]: trap divide by zero error: %#lx\n", myproc()->name, myproc()->pid, tf->rip);
		kill(myproc()->pid, SIGFPE);
		break;
	case T_SIMDERR:
	case T_FPERR:
		uart_printf("%s[%d]: floating point error: %#lx\n", myproc()->name, myproc()->pid, tf->rip);
		kill(myproc()->pid, SIGFPE);
		break;
	default:
		if (myproc() == 0 || (tf->cs & 3) == 0) {
			// In kernel, it must be our mistake.
			uart_printf("unexpected trap %ld from cpu %d rip %lx (cr2=%#lx)\n",
							tf->trapno, my_cpu_id(), tf->rip, rcr2());
			panic("trap");
		}
		// In user space, assume process misbehaved.
		uart_printf("pid %d %s: trap %ld err %ld on cpu %d "
						"rip %#lx addr %#lx--kill proc\n",
						myproc()->pid, myproc()->name, tf->trapno, tf->err, my_cpu_id(),
						tf->rip, rcr2());
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
