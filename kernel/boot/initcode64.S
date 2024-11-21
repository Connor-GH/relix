# Initial process execs /init.

#include "syscall.h"
#include "traps.h"


# exec(init, argv)
.globl start
start:
  mov $init, %rdi
  mov $argv, %rsi
	mov $envp, %rdx
  mov $SYS_execve, %rax
  int $T_SYSCALL

# for(;;) exit();
exit:
  mov $SYS_exit, %rax
  int $T_SYSCALL
  jmp exit

# char init[] = "/init\0";
init:
  .string "/bin/init\0"
env:
	.string "\0"

# char *argv[] = { init, 0 };
.p2align 2
argv:
  .quad init
  .quad 0

envp:
	.quad env
	.quad 0
