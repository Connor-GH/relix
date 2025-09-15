# Initial process execs /bin/init.
# This code runs in user space, but the kernel needs to know about it.

#include "syscall.h"


# exec(init, argv)
.globl start
start:
  mov $init, %rdi
  mov $argv, %rsi
	mov $envp, %rdx
  mov $SYS_execve, %rax
  syscall

# for(;;) _exit(1);
exit:
  mov $1, %rdi
  mov $SYS__exit, %rax
	syscall
  jmp exit

.align 8
# char init[] = "/bin/init\0";
init:
  .string "/bin/init\0"

.align 8
# This is where system-wide default environment variables are set.
env0: .string "PATH=/bin:/usr/bin\0"
# We don't support all of xterm, but let's pretend we do.
env1: .string "TERM=xterm\0"

# char *argv[] = { init, 0 };
.align 8
argv:
  .quad init
  .quad 0

.align 8
envp:
	.quad env0
	.quad env1
	.quad 0
