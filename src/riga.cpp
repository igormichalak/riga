#include "riga.h"

#include <cassert>
#include <expected>
#include <array>
#include <vector>
#include <unordered_map>
#include <bit>
#include <iostream>
#include <format>

#include "riga_bit_vector.h"
#include "riga_opcode.h"
#include "riga_decode.h"

#include "riga_bit_vector.cpp"
#include "riga_opcode.cpp"
#include "riga_decode.cpp"

struct Instruction {
	u8 size;
	Instr_Fmt fmt;
	Opcode opcode;
	u8 rd;
	u8 rs1;
	u8 rs2;
	s64 imm;
	// union {
	// 	struct {
	// 		// U,J,I,R-type -> rd
	// 		// B,S-type -> rs2
	// 		u8 op0;
	// 		// U,J-type -> (4 imm LSB) << 4
	// 		// I,B,S,R-type -> rs1/zimm
	// 		u8 op1;
	// 		// U,J,I,B,S-type -> imm
	// 		// R-type -> rs2
	// 		u16 imm;
	// 	};
	// 	u32 opbits;
	// };
};

struct Standalone_Core {
	// x0-x31 registers.
	std::array<u64, 32> m_register_file{};

	// The only 4 CSRs accessible from within the user-mode.
	// (0) 0xC00 - cycle
	// (1) 0xC01 - time
	// (2) 0xC02 - instret
	// (3) 0xC03 - hpmcounter3
	std::array<u64, 4> m_csrs{};

	static constexpr size_t MEMORY_SIZE = 1024 * 1024;
	std::array<u8, MEMORY_SIZE> m_memory{};

	u64	                     m_pc{0};
	std::vector<Instruction> m_instructions;
	Bit_Vector	             m_instruction_offset_vec;

	// Variably-sized flash/program memory.
	u8	  *m_flash_memory{nullptr};
	size_t m_flash_memory_size{0};

	std::unordered_map<u64, std::string_view> m_symbol_table{};

	auto load_instruction(u32 w) -> std::expected<size_t, Decode_Error>;

	void program_compile();
	void link_flash_memory(void *p, size_t len);
	void define_symbol(u64 value, char const *name);

	char *disassembly_view();

	size_t get_memory_size();
	void *get_memory_ptr();
	void *get_register_file_ptr();

	void run();
	void step();
};

auto Standalone_Core::load_instruction(u32 w)
	-> std::expected<size_t, Decode_Error>
{
	Instruction instr = {};
	instr.size = ((w & 0b11) == 0b11) ? 4 : 2;
	instr.fmt = decode_instr_fmt(w);

	if (const auto opcode = decode_opcode(w); opcode.has_value()) {
		instr.opcode = *opcode;
	} else {
		return std::unexpected(opcode.error());
	}

	switch (instr.fmt) {
	case Instr_Fmt::U:
		instr.imm = imm_u(w);
		instr.rd  = rd(w);
		break;
	case Instr_Fmt::J:
		instr.imm = imm_j(w);
		instr.rd  = rd(w);
		break;
	case Instr_Fmt::I:
		instr.imm = imm_i(w);
		instr.rd  = rd(w);
		instr.rs1 = rs1(w);
		break;
	case Instr_Fmt::B:
		instr.imm = imm_b(w);
		instr.rs1 = rs1(w);
		instr.rs2 = rs2(w);
		break;
	case Instr_Fmt::S:
		instr.imm = imm_s(w);
		instr.rs1 = rs1(w);
		instr.rs2 = rs2(w);
		break;
	case Instr_Fmt::R:
		instr.rd  = rd(w);
		instr.rs1 = rs1(w);
		instr.rs2 = rs2(w);
		break;
	case Instr_Fmt::Unknown:
	default:
		return std::unexpected(Decode_Error::Illegal_Instruction);
	}

	m_instructions.push_back(instr);
	return instr.size;
}

void Standalone_Core::program_compile() {
	if (m_flash_memory == nullptr) return;

	uintptr_t ptr = reinterpret_cast<uintptr_t>(m_flash_memory);
	assert(ptr % alignof(u16) == 0 && "m_flash_memory pointer is not aligned to u16");
	assert(m_flash_memory_size % 2 == 0 && "m_flash_memory_size is not multiple of 2");

	size_t offset = 0;
	while (offset < m_flash_memory_size) {
		if (offset & 1) {
			puts("misaligned instruction access");
			break;
		}
		u16 lhw = *reinterpret_cast<const u16 *>(m_flash_memory + offset);
		u32 w;
		if ((lhw & 0b11) == 0b11 && m_flash_memory_size - offset >= 4) {
			u16 hhw = *reinterpret_cast<const u16 *>(m_flash_memory + offset + 2);
			w = (static_cast<u32>(hhw) << 16) | static_cast<u32>(lhw);
		} else {
			w = static_cast<u32>(lhw);
		}
		if (auto ilen = load_instruction(w); ilen.has_value()) {
			offset += *ilen;
		} else {
			puts("failed to load an instruction");
			break;
		}
	}
}

void Standalone_Core::link_flash_memory(void *p, size_t len) {
	m_flash_memory	    = static_cast<u8 *>(p);
	m_flash_memory_size = len;
}

void Standalone_Core::define_symbol(u64 value, char const *name) {
	m_symbol_table[value] = name;
}

char *Standalone_Core::disassembly_view() {
	std::string output;
	output.reserve(256 * 1024);
	output += "00000000:\n";

	size_t offset = 0;

	for (size_t i = 0; i < m_instructions.size(); ++i) {
		auto &instr = m_instructions[i];

		size_t rel_addr = offset & 0xffff;
		size_t rel_addr_after  = (offset + instr.size) & 0xffff;

		output += std::format(
			"{:4x}:  {}",
			rel_addr,
			to_string(instr.opcode)
		);

		switch (instr.opcode) {
		case Opcode::ecall:
		case Opcode::ebreak:
			break;
		default:
			switch (instr.fmt) {
			case Instr_Fmt::U:
			case Instr_Fmt::J:
			case Instr_Fmt::I:
			case Instr_Fmt::R:
				output += std::format(" {}, ", to_reg_name(instr.rd));
				break;
			case Instr_Fmt::B:
				output += std::format(" {}, ", to_reg_name(instr.rs1));
				break;
			case Instr_Fmt::S:
				output += std::format(" {}, ", to_reg_name(instr.rs2));
				break;
			default: break;
			}

			switch (instr.fmt) {
			case Instr_Fmt::U:
				output += std::format("{:#x}", static_cast<u64>(instr.imm) >> 12);
				break;
			case Instr_Fmt::J:
				output += std::format("{:#x}", offset + instr.imm);
				break;
			case Instr_Fmt::I:
				switch (instr.opcode) {
				case Opcode::jalr:
				case Opcode::lb:
				case Opcode::lh:
				case Opcode::lw:
				case Opcode::ld:
				case Opcode::lbu:
				case Opcode::lhu:
				case Opcode::lwu:
					output += std::format("{}({})", instr.imm, to_reg_name(instr.rs1));
					break;
				case Opcode::csrrw:
				case Opcode::csrrs:
				case Opcode::csrrc:
				case Opcode::csrrwi:
				case Opcode::csrrsi:
				case Opcode::csrrci:
					output += std::format("{:#x}, ", instr.imm & 0xfff);
					break;
				default:
					output += std::format("{}, ", to_reg_name(instr.rs1));
					break;
				}
				break;
			case Instr_Fmt::R:
				output += std::format("{}, ", to_reg_name(instr.rs1));
				break;
			case Instr_Fmt::B:
				output += std::format("{}, ", to_reg_name(instr.rs2));
				break;
			case Instr_Fmt::S:
				output += std::format("{}({})", instr.imm, to_reg_name(instr.rs1));
				break;
			default: break;
			}

			switch (instr.fmt) {
			case Instr_Fmt::I:
				switch (instr.opcode) {
				case Opcode::jalr:
				case Opcode::lb:
				case Opcode::lh:
				case Opcode::lw:
				case Opcode::ld:
				case Opcode::lbu:
				case Opcode::lhu:
				case Opcode::lwu:
					break;
				case Opcode::slli:
				case Opcode::srli:
				case Opcode::srai:
					output += std::format("{}", instr.imm & 0x3f);
					break;
				case Opcode::slliw:
				case Opcode::srliw:
				case Opcode::sraiw:
					output += std::format("{}", instr.imm & 0x1f);
					break;
				case Opcode::csrrw:
				case Opcode::csrrs:
				case Opcode::csrrc:
					output += std::format("{}", to_reg_name(instr.rs1));
					break;
				case Opcode::csrrwi:
				case Opcode::csrrsi:
				case Opcode::csrrci:
					output += std::format("{}", instr.rs1);
					break;
				default:
					output += std::format("{}", instr.imm);
					break;
				}
				break;
			case Instr_Fmt::R:
				output += std::format("{}", to_reg_name(instr.rs2));
				break;
			case Instr_Fmt::B:
				output += std::format("{:#x}", offset + instr.imm);
				break;
			default: break;
			}
		}

		output += "\n";

		if (rel_addr > rel_addr_after && i + 1 < m_instructions.size()) {
			size_t ref_addr = offset & ~static_cast<size_t>(0xffff);
			output += std::format("{:08x}:\n", ref_addr);
		}

		offset += instr.size;
	}

	char *buf = static_cast<char *>(std::malloc(output.size() + 1));
	if (!buf) return nullptr;

	std::memcpy(buf, output.data(), output.size() + 1);
	return buf;
}

size_t Standalone_Core::get_memory_size() {
	return MEMORY_SIZE;
}

void *Standalone_Core::get_memory_ptr() {
	return m_memory.data();
}

void *Standalone_Core::get_register_file_ptr() {
	return m_register_file.data();
}

void Standalone_Core::run() {
	return;
}

void Standalone_Core::step() {
	return;
}
