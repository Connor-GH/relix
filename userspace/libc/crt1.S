.section .text
.global _start
.type _start, @function
/*
 *           %rdi        %rsi         %rdx
 * _start(int argc, char **argv, char **envp)
 */
_start:
	pushq %rbp
	movq %rsp, %rbp
	movabsq $main, %rcx
/*
 *            %rdi        %rsi
 * __libc_start_main(int argc, char **argv,
 *        %rdx       %rcx
 * char **envp, int (*main)(int, char **, char **))
 */
	call __libc_start_main
	int3
