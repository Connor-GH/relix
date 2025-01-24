#include <stdint.h>
#include <stat.h>
#include <elf.h>
#include "param.h"
#include "proc.h"
#include "x86.h"
#include "log.h"
#include "fs.h"
#include "console.h"
#include "kernel_string.h"
#include "kernel_assert.h"
#include "vm.h"
#include "drivers/mmu.h"
#include "compiler_attributes.h"

// count is argc/envc
// vec is argv/envp
static int
push_user_stack(uintptr_t *count, char *const *vec, uintptr_t *ustack,
								uintptr_t *pgdir, uintptr_t *sp, uint32_t idx)
{
	for (*count = 0; vec[*count]; (*count)++) {
		if (*count >= MAXARG)
			return -1;
		// move the stack down to account for an argument
		*sp = (*sp - (strlen(vec[*count]) + 1)) & ~(sizeof(uintptr_t) - 1);
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

__nonnull(1, 2) int execve(const char *path, char *const *argv, char *const *envp)
{
	const char *s, *last;
	int i, off;
	uintptr_t envc = 0;
	uintptr_t argc = 0, sz, sp, ustack[3 + MAXARG + MAXENV + 1] = {};
	struct Elf64_Ehdr elf;
	struct inode *ip;
	struct Elf64_Phdr ph;
	uintptr_t *pgdir, *oldpgdir;
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
	if (elf.magic != ELF_MAGIC_NUMBER)
		goto bad;

	if ((pgdir = setupkvm()) == 0)
		goto bad;

	// Load program into memory.
	sz = 0;
	for (i = 0, off = elf.e_phoff; i < elf.e_phnum; i++, off += sizeof(ph)) {
		if (readi(ip, (char *)&ph, off, sizeof(ph)) != sizeof(ph))
			goto bad;
		if (ph.p_type != PT_LOAD)
			continue;
		if (ph.p_memsz < ph.p_filesz)
			goto bad;
		if (ph.p_vaddr + ph.p_memsz < ph.p_vaddr)
			goto bad;
		if ((sz = allocuvm(pgdir, sz, ph.p_vaddr + ph.p_memsz)) == 0)
			goto bad;
		if (ph.p_vaddr % PGSIZE != 0)
			goto bad;
		if (loaduvm(pgdir, (char *)ph.p_vaddr, ip, ph.p_offset, ph.p_filesz) < 0)
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
	if (push_user_stack(&argc, argv, ustack, pgdir, &sp, 4) < 0)
		goto bad;
	if (push_user_stack(&envc, envp, ustack, pgdir, &sp, argc + 4 + 1) < 0)
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
	const uint32_t argv_size = argc + 1;
	const uint32_t envc_size = envc + 1;

	ustack[0] = 0xffffffff; // fake return PC
	ustack[1] = argc;
	// ustack[3 .. 3 + argv_size] is argv arguments
	ustack[2] = sp - (argv_size + envc_size) * sizeof(uintptr_t); // argv pointer
	// ustack[3 + argv_size + 1 .. 3 + argv_size + 1 + envp_size] is envp arguments
	ustack[3] = sp - (envc_size) * sizeof(uintptr_t);
	uint32_t total_mainargs_size =
		(4 + argv_size + envc_size) * sizeof(uintptr_t);
#ifdef X86_64
	myproc()->tf->rdi = ustack[1];
	myproc()->tf->rsi = ustack[2];
	myproc()->tf->rdx = ustack[3];
#endif
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
	curproc->tf->eip = elf.e_entry; // main
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
