#include "console.h"
#include "drivers/mmu.h"
#include "fcntl_constants.h"
#include "fs.h"
#include "lib/compiler_attributes.h"
#include "log.h"
#include "macros.h"
#include "param.h"
#include "proc.h"
#include "vm.h"
#include "x86.h"
#include <elf.h>
#include <errno.h>
#include <stat.h>
#include <stdint.h>
#include <string.h>

// count is argc/envc
// vec is argv/envp
static int
push_user_stack(uintptr_t *count, char *const *vec, uintptr_t *ustack,
                uintptr_t *pgdir, uintptr_t *sp, uint32_t idx)
{
	for (*count = 0; vec[*count]; (*count)++) {
		if (*count >= MAXARG) {
			return -E2BIG;
		}
		// move the stack down to account for an argument
		*sp = (*sp - (strlen(vec[*count]) + 1)) & ~(sizeof(uintptr_t) - 1);
		// copy this vector index onto the stack pointer finally.
		PROPOGATE_ERR(copyout(pgdir, *sp, vec[*count], strlen(vec[*count]) + 1));
		ustack[idx + *count] = *sp;
	}
	// 0 is fake return address
	// 1 is argc
	// 2 is argv pointer (that points to the items on the stack)
	// argv copy will be idx = 3, envp will be idx = 3 + argv_size + 1
	ustack[idx + *count] = 0;
	return 0;
}

__nonnull(1, 2) int execve(const char *path, char *const *argv,
                           char *const *envp)
{
	const char *s, *last;
	uintptr_t envc = 0;
	uintptr_t argc = 0, sp, ustack[3 + MAXARG + MAXENV + 1] = {};
	struct Elf64_Ehdr elf;
	struct inode *ip;
	struct Elf64_Phdr ph;
	uintptr_t *pgdir = NULL;
	uintptr_t *oldpgdir;
	int return_errno = 0, errno_tmp = 0;
	struct proc *curproc = myproc();

	begin_op();

	if ((ip = namei(path)) == NULL) {
		end_op();
		return -ENOENT;
	}
	inode_lock(ip);

	// hold back on GID/UID protection right now
	/*if (ip->gid != curproc->cred.gid && ip->uid != curproc->cred.uid) {
	  cprintf("exec: user does not have matching uid/gid for this file\n");
	  cprintf("Requested gid: %d user gid: %d\n", ip->gid, curproc->cred.gid);
	  cprintf("Requested uid: %d user uid: %d\n", ip->uid, curproc->cred.uid);
	  inode_unlockput(ip);
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
		inode_unlockput(ip);
		return -EACCES;
	}

ok:
	// Check ELF header
	if ((errno_tmp = inode_read(ip, (char *)&elf, 0, sizeof(elf))) < 0) {
		return_errno = errno_tmp;
		goto bad;
	}
	if (elf.magic != ELF_MAGIC_NUMBER) {
		goto bad;
	}

	// We require 64 bit, little endian, linux/unix ELF files.
	if (elf.e_ident[EI_CLASS] != ELFCLASS64) {
		goto bad;
	}

	if (elf.e_ident[EI_DATA] != ELFDATA2LSB) {
		goto bad;
	}

	if (elf.e_ident[EI_OSABI] != ELFOSABI_SYSV &&
	    elf.e_ident[EI_OSABI] != ELFOSABI_LINUX) {
		goto bad;
	}

	if (elf.e_ident[EI_VERSION] != EV_CURRENT) {
		goto bad;
	}

	if ((pgdir = setupkvm()) == NULL) {
		return_errno = -ENOMEM;
		goto bad;
	}

	uintptr_t sz = 0;
	// Load program into memory.
	for (size_t i = 0, off = elf.e_phoff; i < elf.e_phnum;
	     i++, off += sizeof(ph)) {
		if ((errno_tmp = inode_read(ip, (char *)&ph, off, sizeof(ph))) < 0) {
			return_errno = errno_tmp;
			goto bad;
		}
		if (ph.p_type != PT_LOAD) {
			continue;
		}

		// Skip empty segments.
		if (ph.p_memsz == 0) {
			continue;
		}

#if FULL_ELF_SUPPORT
		if (ph.p_align % PGSIZE != 0) {
			goto bad;
		}
#endif

		if (ph.p_vaddr % ph.p_align != ph.p_offset % ph.p_align) {
			goto bad;
		}

		if (ph.p_memsz < ph.p_filesz) {
			goto bad;
		}
		if (ph.p_vaddr + ph.p_memsz < ph.p_vaddr) {
			goto bad;
		}
		if ((sz = allocuvm(pgdir, sz, ph.p_vaddr + ph.p_memsz)) == 0) {
			return_errno = -ENOMEM;
			goto bad;
		}
		if ((errno_tmp = loaduvm(pgdir, (char *)ph.p_vaddr, ip, ph.p_offset,
		                         ph.p_filesz)) < 0) {
			return_errno = errno_tmp;
			goto bad;
		}
	}
	inode_unlockput(ip);
	end_op();
	ip = NULL;
	// Allocate two pages at the next page boundary.
	// Make the first inaccessible.  Use the second as the user stack.
	sz = PGROUNDUP(sz);
	if ((sz = allocuvm(pgdir, sz, sz + 4 * PGSIZE)) == 0) {
		return_errno = -ENOMEM;
		goto bad;
	}
	clearpteu(pgdir, (char *)(sz - 4 * PGSIZE));
	// Make NULL dereferences cause a page fault.
	clearpteu(pgdir, (char *)0);
	sp = sz;

	// Push argument strings, prepare rest of stack in ustack.
	if (push_user_stack(&argc, argv, ustack, pgdir, &sp, 4) < 0) {
		return_errno = -EFAULT;
		goto bad;
	}
	if (push_user_stack(&envc, envp, ustack, pgdir, &sp, argc + 4 + 1) < 0) {
		return_errno = -EFAULT;
		goto bad;
	}

	/*
	 * 0 = fake address
	 * ^ this is what gets popped off the stack when we try to return.
	 * 1 = argc
	 * ^ this is where main starts popping arguments off the stack
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
	// ustack[3 + argv_size + 1 .. 3 + argv_size + 1 + envp_size] is envp
	// arguments
	ustack[3] = sp - (envc_size) * sizeof(uintptr_t);
	uint32_t total_mainargs_size =
		(4 + argv_size + envc_size) * sizeof(uintptr_t);
#ifdef X86_64
	myproc()->tf->rdi = ustack[1];
	myproc()->tf->rsi = ustack[2];
	myproc()->tf->rdx = ustack[3];
#endif
	sp -= total_mainargs_size;
	if (copyout(pgdir, sp, ustack, total_mainargs_size) < 0) {
		return_errno = -EFAULT;
		goto bad;
	}

	// Save program name for debugging.
	for (last = s = path; *s; s++) {
		if (*s == '/') {
			last = s + 1;
		}
	}
	__safestrcpy(curproc->name, last, sizeof(curproc->name));

	// Commit to the user image.
	oldpgdir = curproc->pgdir;
	curproc->pgdir = pgdir;
	curproc->sz = sz;
	curproc->tf->rip = elf.e_entry; // main

	// FIXME does this corrupt the stack?
	// Round up because we might not always have an already-rounded
	// stack here.
	// The "-8" is needed for alignment to 16 bytes.
	// Without it, we are only aligned to 8 bytes.
	curproc->tf->rsp = ROUND_UP(sp, 16) - 8;
	memset(curproc->mmap_info, 0, sizeof(curproc->mmap_info));
	curproc->mmap_count = 0;

	// If parent is NULL, it's also possible we are init.
	// TODO this needs to be a copy, not a reference
	if (curproc->parent != NULL) {
		curproc->cred = curproc->parent->cred;
	}

	// Only close files if we were passed FD_CLOEXEC.
	for (int i = 0; i < NOFILE; i++) {
		if (curproc->ofile[i] != NULL && curproc->ofile[i]->flags == FD_CLOEXEC) {
			(void)fileclose(curproc->ofile[i]);
			// This is needed as fileclose() does not do this.
			curproc->ofile[i] = NULL;
		}
	}

	switchuvm(curproc);
	freevm(oldpgdir);
	return 0;

bad:
	if (pgdir) {
		freevm(pgdir);
	}
	if (ip) {
		inode_unlockput(ip);
		end_op();
	}
	if (return_errno == 0) {
		return -ENOEXEC;
	} else {
		return return_errno;
	}
}
