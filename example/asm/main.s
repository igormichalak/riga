.text
.align 2
.globl _entry_point
_entry_point:
	j main

main:
	addi a0, a0, 1
	addi a1, a1, -2
	addi a2, a2, 4
	addi a3, a3, -8
	addi a4, a4, 16
	addi a5, a5, -32
	j main

