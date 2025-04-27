enum class Opcode : u16 {
	// RVI
	lui,    // U
	auipc,  // U
	jal,    // J
	jalr,   // I
	beq,    // B
	bne,    // B
	blt,    // B
	bge,    // B
	bltu,   // B
	bgeu,   // B
	lb,     // I
	lh,     // I
	lw,     // I
	lbu,    // I
	lhu,    // I
	sb,     // S 0100011
	sh,     // S 0100011
	sw,     // S 0100011
	addi,   // I
	slti,   // I
	sltiu,  // I
	xori,   // I
	ori,    // I
	andi,   // I
	slli,   // I
	srli,   // I
	srai,   // I
	add,    // R
	sub,    // R
	sll,    // R
	slt,    // R
	sltu,   // R
	xor_,   // R
	srl,    // R
	sra,    // R
	or_,    // R
	and_,   // R
	ecall,  // I
	ebreak, // I
	csrrw,  // I
	csrrs,  // I
	csrrc,  // I
	csrrwi, // I
	csrrsi, // I
	csrrci, // I

	// RVM
	mul,    // R
	mulh,   // R
	mulhsu, // R
	mulhu,  // R
	div,    // R
	divu,   // R
	rem,    // R
	remu,   // R

	// RV64I
	lwu,   // I
	ld,    // I
	sd,    // S 0100011
	addiw, // I
	slliw, // I
	srliw, // I
	sraiw, // I
	addw,  // R
	subw,  // R
	sllw,  // R
	srlw,  // R
	sraw,  // R

	// RV64
	mulw,  // R
	divw,  // R
	divuw, // R
	remw,  // R
	remuw, // R
};

std::string_view to_string(Opcode opcode) noexcept;
