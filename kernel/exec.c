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

__nonnull(1, 2)
int
exec(const char *path, const char **argv)
{
	char *s, *last;
	int i, off;
	uint argc, sz, sp, ustack[3 + MAXARG + 1];
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
	/*if (ip->gid != curproc->cred->gid && ip->uid != curproc->cred->uid) {
    cprintf("exec: user does not have matching uid/gid for this file\n");
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
	for (argc = 0; argv[argc]; argc++) {
		if (argc >= MAXARG)
			goto bad;
		sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
		if (copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
			goto bad;
		ustack[3 + argc] = sp;
	}
	ustack[3 + argc] = 0;

	ustack[0] = 0xffffffff; // fake return PC
	ustack[1] = argc;
	ustack[2] = sp - (argc + 1) * 4; // argv pointer

	sp -= (3 + argc + 1) * 4;
	if (copyout(pgdir, sp, ustack, (3 + argc + 1) * 4) < 0)
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
