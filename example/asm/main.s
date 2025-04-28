.text
.align 2
.globl _entry_point
_entry_point:
	j main

main:
	lui x0, 0xfffff
	auipc x0, 20
	sw a0, 4(sp)
	sw a1, 8(sp)
	sw a2, -4(sp)
	sw a3, -8(sp)
	j main
	j main
	j main
	j main
	j main
	j main
	ecall
	ebreak
	csrrwi t0, 0xfff, 8
	csrrwi t1, 0xf00, 2
	csrrwi t2, 0x07f, 10
	add a0, a1, a2
	beq x0, x0, main
	bne a0, a1, main
	bne t0, t1, main
	addi t0, t0, 20
	addi t0, t1, -80
	sltu a0, a1, 1024
	slli x0, x0, 8
	srli x0, x0, 4
	srai x0, x0, 63
	slliw x0, x0, 8
	srliw x0, x0, 4
	sraiw x0, x0, 31
