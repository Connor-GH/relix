/*
 * This is the front of _init and _fini that
 * gets placed before their contents.
 */
.section .init, "ax", @progbits
.global _init
.align 8
.type _init, @function

_init:
	pushq %rbp
	movq %rsp, %rbp

.section .fini, "ax", @progbits
.global _fini
.align 8
.type _fini, @function
_fini:
	pushq %rbp
	movq %rsp, %rbp

.section .note.GNU-stack, "", %progbits
