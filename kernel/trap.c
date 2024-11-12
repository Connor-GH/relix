#include <stdlib.h>
#include <stdint.h>
#include "drivers/mmu.h"
#include "drivers/lapic.h"
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

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uintptr_t vectors[]; // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint32_t ticks;

#ifndef X86_64
void
tvinit(void)
{
	int i;

	for (i = 0; i < 256; i++)
		SETGATE(idt[i], 0, SEG_KCODE << 3, vectors[i], 0);
	SETGATE(idt[T_SYSCALL], 1, SEG_KCODE << 3, vectors[T_SYSCALL], DPL_USER);

	initlock(&tickslock, "time");
}

void
idtinit(void)
{
	lidt(idt, sizeof(idt));
}
#endif

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
	case T_IRQ0 + 7:
	case T_IRQ0 + IRQ_SPURIOUS:
		cprintf("cpu%d: spurious interrupt at %#lx:%#lx\n", my_cpu_id(), tf->cs,
						tf->eip);
		lapiceoi();
		break;
	case T_ILLOP:
		cprintf("Illegal instruction\n");
		myproc()->killed = 1;
		break;
	case T_GPFLT:
		cprintf("General protection fault\n");
		myproc()->killed = 1;
		break;
	// TODO handle pagefaults in a way that allows copy-on-write
	case T_PGFLT:
		cprintf("Page fault at %#lx\n", rcr2());
		myproc()->killed = 1;
		break;
	case T_FPERR:
	case T_SIMDERR:
		cprintf("Floating point error\n");
		myproc()->killed = 1;
		break;
	case T_DIVIDE:
		cprintf("Division by zero.\n");
		myproc()->killed = 1;
		break;

	default:
		if (myproc() == 0 || (tf->cs & 3) == 0) {
			// In kernel, it must be our mistake.
			cprintf("unexpected trap %ld from cpu %d eip %lx (cr2=%#lx)\n", tf->trapno,
							my_cpu_id(), tf->eip, rcr2());
			panic("trap");
		}
		// In user space, assume process misbehaved.
		cprintf("pid %d %s: trap %ld err %ld on cpu %d "
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
