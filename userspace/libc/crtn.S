/*
 * This is the tail end of _init and _fini that
 * gets placed after their contents.
 */

.section .init, "ax", @progbits
	popq %rbp
	ret

.section .fini, "ax", @progbits
	popq %rbp
	ret

.section .note.GNU-stack, "", %progbits
