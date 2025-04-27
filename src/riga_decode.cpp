consteval bool wc_match(u8 field, char const pattern[6]) {
	for (size_t b = 0; b < 5; ++b) {
		char p = pattern[4 - b];
		if (p == 'x') continue;
		if (((field >> b) & 1) != (p - '0')) return false;
	}
	return true;
}

consteval Instr_Fmt decode_field(u8 f) {
	if (wc_match(f, "0x101")) return Instr_Fmt::U;
	if (wc_match(f, "11011")) return Instr_Fmt::J;
	if (wc_match(f, "11000")) return Instr_Fmt::B;
	if (wc_match(f, "01000")) return Instr_Fmt::S;
	if (wc_match(f, "011x0")) return Instr_Fmt::R;

	if (wc_match(f, "00x00") ||
	    wc_match(f, "00110") ||
	    wc_match(f, "11001") ||
	    wc_match(f, "11100")) return Instr_Fmt::I;

	return Instr_Fmt::Unknown;
}

consteval auto make_decode_lut() {
	std::array<Instr_Fmt, 32> lut{};
	for (size_t i = 0; i < 32; ++i) {
		lut[i] = decode_field(static_cast<u8>(i));
	}
	return lut;
}

static constexpr auto decode_lut = make_decode_lut();

inline Instr_Fmt decode_instr_fmt(u32 w) {
	return decode_lut[static_cast<u8>((w >> 2) & 0x1F)];
}

static constexpr std::array<std::string_view, 7> instr_fmt_names = {{
	"U", "J", "I", "B", "S", "R", "Unknown"
}};

std::string_view to_string(Instr_Fmt fmt) noexcept {
	return instr_fmt_names[static_cast<size_t>(fmt)];
}

inline u8 opcode (u32 w) { return w & 0x7f; }
inline u8 funct3 (u32 w) { return (w >> 12) & 0x07; }
inline u8 funct7 (u32 w) { return (w >> 25) & 0x7f; }
inline u8 rd     (u32 w) { return (w >> 7)  & 0x1f; }
inline u8 rs1    (u32 w) { return (w >> 15) & 0x1f; }
inline u8 rs2    (u32 w) { return (w >> 20) & 0x1f; }

template <unsigned N>
constexpr s64 sign_extend(u32 w) {
	static_assert(N > 0 && N < 32, "invalid bit-width");
	const u32 m = 1U << (N - 1);
	return static_cast<s64>((w ^ m) - m);
}

inline s64 imm_i(u32 w) {
	return sign_extend<12>(w >> 20);
}

inline s64 imm_s(u32 w) {
	u32 imm = ((w >> 7)  & 0x1f) |
		      ((w >> 25) & 0x7f) << 5;
	return sign_extend<12>(imm);
}

inline s64 imm_b(u32 w) {
	u32 imm = ((w >> 31) & 0x01) << 12 |
		      ((w >> 7)  & 0x01) << 11 |
		      ((w >> 25) & 0x3f) << 5  |
		      ((w >> 8)  & 0x0f) << 1;
	return sign_extend<13>(imm);
}

inline s64 imm_u(u32 w) {
	return static_cast<s64>(w & 0xfffff000U);
}

inline s64 imm_j(u32 w) {
	u32 imm = ((w >> 31) & 0x001) << 20 |
		      ((w >> 12) & 0x0ff) << 12 |
		      ((w >> 20) & 0x001) << 11 |
		      ((w >> 21) & 0x3ff) << 1;
	return sign_extend<21>(imm);
}

auto decode_opcode(u32 w) -> std::expected<Opcode, Decode_Error> {
	switch (opcode(w)) {
	case 0x03:
		switch (funct3(w)) {
		case 0b000: return Opcode::lb;
		case 0b001: return Opcode::lh;
		case 0b010: return Opcode::lw;
		case 0b011: return Opcode::ld;
		case 0b100: return Opcode::lbu;
		case 0b101: return Opcode::lhu;
		case 0b110: return Opcode::lwu;
		default: break;
		}
		break;
	case 0x13:
		switch (funct3(w)) {
		case 0b000: return Opcode::addi;
		case 0b001: return Opcode::slli;
		case 0b010: return Opcode::slti;
		case 0b011: return Opcode::sltiu;
		case 0b100: return Opcode::xori;
		case 0b101: return (funct7(w) == 0) ? Opcode::srli : Opcode::srai;
		case 0b110: return Opcode::ori;
		case 0b111: return Opcode::andi;
		default: break;
		}
		break;
	case 0x17:
		return Opcode::auipc;
	case 0x1b:
		switch (funct3(w)) {
		case 0b000: return Opcode::addiw;
		case 0b001: return Opcode::slliw;
		case 0b101: return (funct7(w) == 0) ? Opcode::srliw : Opcode::sraiw;
		default: break;
		}
		break;
	case 0x23:
		switch (funct3(w)) {
		case 0b000: return Opcode::sb;
		case 0b001: return Opcode::sh;
		case 0b010: return Opcode::sw;
		case 0b011: return Opcode::sd;
		default: break;
		}
		break;
	case 0x33:
		switch (funct7(w) & 1) {
		case 0:
			switch (funct3(w)) {
			case 0b000: return (funct7(w) == 0) ? Opcode::add : Opcode::sub;
			case 0b001: return Opcode::sll;
			case 0b010: return Opcode::slt;
			case 0b011: return Opcode::sltu;
			case 0b100: return Opcode::xor_;
			case 0b101: return (funct7(w) == 0) ? Opcode::srl : Opcode::sra;
			case 0b110: return Opcode::or_;
			case 0b111: return Opcode::and_;
			default: break;
			}
			break;
		case 1:
			switch (funct3(w)) {
			case 0b000: return Opcode::mul;
			case 0b001: return Opcode::mulh;
			case 0b010: return Opcode::mulhsu;
			case 0b011: return Opcode::mulhu;
			case 0b100: return Opcode::div;
			case 0b101: return Opcode::divu;
			case 0b110: return Opcode::rem;
			case 0b111: return Opcode::remu;
			default: break;
			}
			break;
		}
		break;
	case 0x37:
		return Opcode::lui;
	case 0x3b:
		switch (funct7(w) & 1) {
		case 0:
			switch (funct3(w)) {
			case 0b000: return (funct7(w) == 0) ? Opcode::addw : Opcode::subw;
			case 0b001: return Opcode::sllw;
			case 0b101: return (funct7(w) == 0) ? Opcode::srlw : Opcode::sraw;
			default: break;
			}
			break;
		case 1:
			switch (funct3(w)) {
			case 0b000: return Opcode::mulw;
			case 0b100: return Opcode::divw;
			case 0b101: return Opcode::divuw;
			case 0b110: return Opcode::remw;
			case 0b111: return Opcode::remuw;
			default: break;
			}
			break;
		}
		break;
	case 0x63:
		switch (funct3(w)) {
		case 0b000: return Opcode::beq;
		case 0b001: return Opcode::bne;
		case 0b100: return Opcode::blt;
		case 0b101: return Opcode::bge;
		case 0b110: return Opcode::bltu;
		case 0b111: return Opcode::bgeu;
		default: break;
		}
		break;
	case 0x67:
		return Opcode::jalr;
	case 0x6f:
		return Opcode::jal;
	default: break;
	}

	return std::unexpected(Decode_Error::Illegal_Instruction);
}
