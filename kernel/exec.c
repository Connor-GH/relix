#include <types.h>
#include <stat.h>
#include <defs.h>
#include "param.h"
#include "proc.h"
#include "x86.h"
#include "log.h"
#include "fs.h"
#include "console.h"
#include "kernel_string.h"
#include "vm.h"
#include "boot/elf.h"
#include "drivers/mmu.h"
#include "compiler_attributes.h"

// count is argc/envc
// vec is argv/envp
static int
push_user_stack(uint *count, char **vec, uint *ustack, pde_t *pgdir, uint *sp,
								uint idx)
{
	for (*count = 0; vec[*count]; (*count)++) {
		if (*count >= MAXARG)
			return -1;
		// move the stack down to account for an argument
		*sp -= strlen(vec[*count]) + 1;
		// (seemingly) align the stack to 4 bytes.
		*sp &= ~(sizeof(uintptr_t) - 1);
		// copy this vector index onto the stack pointer finally.
		if (copyout(pgdir, *sp, vec[*count], strlen(vec[*count]) + 1) < 0)
			return -1;
		ustack[idx + *count] = *sp;
	}
	// 0 is fake return address
	// 1 is argc
	// 2 is argv pointer (that points to the items on the stack)
	// argv copy will be idx = 3, envp will be idx = 3 + argv_size + 1
	ustack[idx + *count] = 0;
	return 0;
}

__nonnull(1, 2) int exec(char *path, char **argv)
{
	char *s, *last;
	int i, off;
	uint argc = 0, sz, sp, ustack[3 + MAXARG + MAXENV + 1] = {};
	struct elfhdr elf;
	struct inode *ip;
	struct proghdr ph;
	pde_t *pgdir, *oldpgdir;
	struct proc *curproc = myproc();

	begin_op();

	if ((ip = namei(path)) == 0) {
		end_op();
		return -1;
	}
	ilock(ip);
	pgdir = 0;

	// hold back on GID/UID protection right now
	/*if (ip->gid != curproc->cred.gid && ip->uid != curproc->cred.uid) {
		cprintf("exec: user does not have matching uid/gid for this file\n");
		cprintf("Requested gid: %d user gid: %d\n", ip->gid, curproc->cred.gid);
		cprintf("Requested uid: %d user uid: %d\n", ip->uid, curproc->cred.uid);
		iunlockput(ip);
		return -1;
	}*/
	// TODO change "1" to check for user permissions
	// add back when proper file permissions are added.
	if (!S_HASPERM(ip->mode, S_IXUSR)) {
		// if we're in the right group, we're fine.
		// if we have group privs but not in the
		// right group, this fails.
		if (S_HASPERM(ip->mode, S_IXGRP)) {
			if (ip->gid & 1) {
				goto ok;
			}
			cprintf("exec: user is not in group %d\n", ip->gid);
		}
		end_op();
		cprintf("exec: file is not executable\n");
		iunlockput(ip);
		return -1;
	}

ok:
	// Check ELF header
	if (readi(ip, (char *)&elf, 0, sizeof(elf)) != sizeof(elf))
		goto bad;
	if (elf.magic != ELF_MAGIC)
		goto bad;

	if ((pgdir = setupkvm()) == 0)
		goto bad;

	// Load program into memory.
	sz = 0;
	for (i = 0, off = elf.phoff; i < elf.phnum; i++, off += sizeof(ph)) {
		if (readi(ip, (char *)&ph, off, sizeof(ph)) != sizeof(ph))
			goto bad;
		if (ph.type != ELF_PROG_LOAD)
			continue;
		if (ph.memsz < ph.filesz)
			goto bad;
		if (ph.vaddr + ph.memsz < ph.vaddr)
			goto bad;
		if ((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
			goto bad;
		if (ph.vaddr % PGSIZE != 0)
			goto bad;
		if (loaduvm(pgdir, (char *)ph.vaddr, ip, ph.off, ph.filesz) < 0)
			goto bad;
	}
	iunlockput(ip);
	end_op();
	ip = 0;

	// Allocate two pages at the next page boundary.
	// Make the first inaccessible.  Use the second as the user stack.
	sz = PGROUNDUP(sz);
	if ((sz = allocuvm(pgdir, sz, sz + 2 * PGSIZE)) == 0)
		goto bad;
	clearpteu(pgdir, (char *)(sz - 2 * PGSIZE));
	sp = sz;

	// Push argument strings, prepare rest of stack in ustack.
	if (push_user_stack(&argc, argv, ustack, pgdir, &sp, 3) < 0)
		goto bad;

	/*
	* 0 = fake address
	* ^ this is what gets popped off the stack when we try to return.
	* 1 = argc
	* & this is where main starts popping arguments off the stack
	* 2 = argv
	* 3 = envp
	* 4 = argv_data ...
	* ^ we need argv to point to this segment of memory
	* 5 = envp_data ...
	*/
	const uint argv_size = argc + 1;

	ustack[0] = 0xffffffff; // fake return PC
	ustack[1] = argc;
	ustack[2] = sp - (argv_size) * 4; // argv pointer
	// ustack[3 .. 3 + argv_size] is argv arguments
	uint total_mainargs_size = (3 + argv_size) * sizeof(uintptr_t);
	sp -= total_mainargs_size;
	if (copyout(pgdir, sp, ustack, total_mainargs_size) < 0)
		goto bad;

	// Save program name for debugging.
	for (last = s = path; *s; s++)
		if (*s == '/')
			last = s + 1;
	safestrcpy(curproc->name, last, sizeof(curproc->name));

	// Commit to the user image.
	oldpgdir = curproc->pgdir;
	curproc->pgdir = pgdir;
	curproc->sz = sz;
	curproc->tf->eip = elf.entry; // main
	curproc->tf->esp = sp;
	curproc->cred = curproc->parent->cred;
	switchuvm(curproc);
	freevm(oldpgdir);
	return 0;

bad:
	if (pgdir)
		freevm(pgdir);
	if (ip) {
		iunlockput(ip);
		end_op();
	}
	return -1;
}
