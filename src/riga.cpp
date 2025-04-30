#include "riga.h"

#include <cassert>
#include <expected>
#include <array>
#include <vector>
#include <unordered_map>
#include <algorithm>
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
	size_t	                 m_ic{0};
	std::vector<Instruction> m_instructions;
	Bit_Vector	             m_instruction_offset_vec;

	// Variably-sized flash/program memory.
	u8	  *m_flash_memory{nullptr};
	size_t m_flash_memory_size{0};

	std::unordered_map<u64, std::string_view> m_symbol_table{};
	std::vector<u32> m_disassembly_view_map{};
	std::array<Instruction, 128> m_break_instructions{};

	// if set it gets executed instead of usual m_instractions[m_ic],
	// then gets cleared
	Instruction *m_next_instruction_override = nullptr;

	void init();

	auto load_instruction(u32 w) -> std::expected<size_t, Decode_Error>;

	void program_compile();
	void link_flash_memory(void *p, size_t len);
	void define_symbol(u64 value, char const *name);

	u32 const *disassembly_view_map();
	char const *disassembly_view();

	size_t get_memory_size();
	void *get_memory_ptr();
	void *get_register_file_ptr();

	void *get_pc_ptr();
	void *get_ic_ptr();
	bool execute_next_instruction();

	bool set_breakpoint(size_t instr_idx);
	bool clear_breakpoint(size_t instr_idx);

	void reset();
	void run();
	void step();
};

void Standalone_Core::init()
{
	for (size_t i = 0; i < m_break_instructions.size(); ++i) {
		m_break_instructions[i].fmt    = Instr_Fmt::I;
		m_break_instructions[i].opcode = Opcode::ebreak;
	}
}

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

u32 const *Standalone_Core::disassembly_view_map() {
	return m_disassembly_view_map.data();
}

char const *Standalone_Core::disassembly_view() {
	static std::string output = []{
		std::string s;
		s.reserve(256 * 1024);
		return s;
	}();

	output = "00000000:\n";
	size_t offset = 0;

	m_disassembly_view_map.assign(m_instructions.size() + 1, 0);

	for (size_t i = 0; i < m_instructions.size(); ++i) {
		auto &instr = m_instructions[i];

		size_t rel_addr = offset & 0xffff;
		size_t rel_addr_after  = (offset + instr.size) & 0xffff;

		m_disassembly_view_map[i] = output.length();
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

	return output.c_str();
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

size_t calculate_ic(u64 pc) {
	return 0;
}

size_t get_address(u64 base, s64 offset) {
	s64 signed_addr = static_cast<s64>(base) + offset;
	return static_cast<size_t>(signed_addr);
}

bool Standalone_Core::execute_next_instruction() {
	if (m_ic == m_instructions.size()) {
		return false;
	}

	Instruction &instr = m_next_instruction_override
		? *m_next_instruction_override
		:  m_instructions[m_ic];

	m_next_instruction_override = nullptr;

	bool advance_pc = true;

	s64 rs1  = static_cast<s64>(m_register_file[instr.rs1]);
	s64 rs2  = static_cast<s64>(m_register_file[instr.rs2]);
	u64 rs1u = m_register_file[instr.rs1];
	u64 rs2u = m_register_file[instr.rs2];

	switch (instr.opcode) {
	case Opcode::lui: {
		m_register_file[instr.rd] = instr.imm;
		break;
	}
	case Opcode::auipc: {
		m_register_file[instr.rd] = get_address(m_pc, instr.imm);
		break;
	}
	case Opcode::jal: {
		m_register_file[instr.rd] = m_pc + instr.size;
		m_pc = get_address(m_pc, instr.imm);
		m_ic = calculate_ic(m_pc);
		advance_pc = false;
		break;
	}
	case Opcode::jalr:{
		m_register_file[instr.rd] = m_pc + instr.size;
		m_pc = get_address(rs1u, instr.imm) & ~1ULL;
		m_ic = calculate_ic(m_pc);
		advance_pc = false;
		break;
	}
	case Opcode::beq: {
		if (rs1 == rs2) {
			m_pc = get_address(m_pc, instr.imm);
			m_ic = calculate_ic(m_pc);
			advance_pc = false;
		}
		break;
	}
	case Opcode::bne: {
		if (rs1 != rs2) {
			m_pc = get_address(m_pc, instr.imm);
			m_ic = calculate_ic(m_pc);
			advance_pc = false;
		}
		break;
	}
	case Opcode::blt: {
		if (rs1 < rs2) {
			m_pc = get_address(m_pc, instr.imm);
			m_ic = calculate_ic(m_pc);
			advance_pc = false;
		}
		break;
	}
	case Opcode::bltu: {
		if (rs1u < rs2u) {
			m_pc = get_address(m_pc, instr.imm);
			m_ic = calculate_ic(m_pc);
			advance_pc = false;
		}
		break;
	}
	case Opcode::bge: {
		if (rs1 >= rs2) {
			m_pc = get_address(m_pc, instr.imm);
			m_ic = calculate_ic(m_pc);
			advance_pc = false;
		}
		break;
	}
	case Opcode::bgeu: {
		if (rs1u >= rs2u) {
			m_pc = get_address(m_pc, instr.imm);
			m_ic = calculate_ic(m_pc);
			advance_pc = false;
		}
		break;
	}
	case Opcode::lb: {
		size_t address = get_address(rs1u, instr.imm);
		m_register_file[instr.rd] = sign_extend<8>(m_memory[address]);
		break;
	}
	case Opcode::lbu: {
		size_t address = get_address(rs1u, instr.imm);
		m_register_file[instr.rd] = m_memory[address];
		break;
	}
	case Opcode::lh: {
		size_t address = get_address(rs1u, instr.imm);
		assert(address % alignof(u16) == 0 && "unaligned load");
		u16 v = *reinterpret_cast<u16 *>(m_memory.data() + address);
		m_register_file[instr.rd] = sign_extend<16>(v);
		break;
	}
	case Opcode::lhu: {
		size_t address = get_address(rs1u, instr.imm);
		assert(address % alignof(u16) == 0 && "unaligned load");
		u16 v = *reinterpret_cast<u16 *>(m_memory.data() + address);
		m_register_file[instr.rd] = v;
		break;
	}
	case Opcode::lw: {
		size_t address = get_address(rs1u, instr.imm);
		assert(address % alignof(u32) == 0 && "unaligned load");
		u32 v = *reinterpret_cast<u32 *>(m_memory.data() + address);
		m_register_file[instr.rd] = sign_extend<32>(v);
		break;
	}
	case Opcode::lwu: {
		size_t address = get_address(rs1u, instr.imm);
		assert(address % alignof(u32) == 0 && "unaligned load");
		u32 v = *reinterpret_cast<u32 *>(m_memory.data() + address);
		m_register_file[instr.rd] = v;
		break;
	}
	case Opcode::ld: {
		size_t address = get_address(rs1u, instr.imm);
		assert(address % alignof(u64) == 0 && "unaligned load");
		u64 v = *reinterpret_cast<u64 *>(m_memory.data() + address);
		m_register_file[instr.rd] = v;
		break;
	}
	case Opcode::sb: {
		size_t address = get_address(rs1u, instr.imm);
		m_memory[address] = rs2u & 0xff;
		break;
	}
	case Opcode::sh: {
		size_t address = get_address(rs1u, instr.imm);
		assert(address % alignof(u16) == 0 && "unaligned store");
		u16 *dst = reinterpret_cast<u16 *>(m_memory.data() + address);
		*dst = rs2u & 0xffff;
		break;
	}
	case Opcode::sw: {
		size_t address = get_address(rs1u, instr.imm);
		assert(address % alignof(u32) == 0 && "unaligned store");
		u32 *dst = reinterpret_cast<u32 *>(m_memory.data() + address);
		*dst = rs2u & 0xffffffff;
		break;
	}
	case Opcode::sd: {
		size_t address = get_address(rs1u, instr.imm);
		assert(address % alignof(u64) == 0 && "unaligned store");
		u64 *dst = reinterpret_cast<u64 *>(m_memory.data() + address);
		*dst = rs2u;
		break;
	}
	case Opcode::add: {
		m_register_file[instr.rd] = rs1u + rs2u;
		break;
	}
	case Opcode::addi: {
		m_register_file[instr.rd] = rs1 + instr.imm;
		break;
	}
	case Opcode::addw: {
		u64 v = sign_extend<32>((rs1u + rs2u) & 0xffffffff);
		m_register_file[instr.rd] = v;
		break;
	}
	case Opcode::addiw: {
		u64 v = sign_extend<32>((rs1 + instr.imm) & 0xffffffff);
		m_register_file[instr.rd] = v;
		break;
	}
	case Opcode::slt: {
		u64 v = (rs1 < rs2) ? 1 : 0;
		m_register_file[instr.rd] = v;
		break;
	}
	case Opcode::slti: {
		u64 v = (rs1 < instr.imm) ? 1 : 0;
		m_register_file[instr.rd] = v;
		break;
	}
	case Opcode::sltu: {
		u64 v = (rs1u < rs2u) ? 1 : 0;
		m_register_file[instr.rd] = v;
		break;
	}
	case Opcode::sltiu: {
		u64 v = (rs1u < static_cast<u64>(instr.imm)) ? 1 : 0;
		m_register_file[instr.rd] = v;
		break;
	}
	case Opcode::sll: {
		m_register_file[instr.rd] = rs1u << (rs2u & 0x3f);
		break;
	}
	case Opcode::slli: {
		m_register_file[instr.rd] = rs1u << (instr.imm & 0x3f);
		break;
	}
	case Opcode::sllw: {
		u64 v = (rs1u << (rs2u & 0x1f)) & 0xffffffff;
		m_register_file[instr.rd] = sign_extend<32>(v);
		break;
	}
	case Opcode::slliw: {
		u64 v = (rs1u << (instr.imm & 0x1f)) & 0xffffffff;
		m_register_file[instr.rd] = sign_extend<32>(v);
		break;
	}
	case Opcode::srl: {
		m_register_file[instr.rd] = rs1u >> (rs2u & 0x3f);
		break;
	}
	case Opcode::srli: {
		m_register_file[instr.rd] = rs1u >> (instr.imm & 0x3f);
		break;
	}
	case Opcode::srlw: {
		u64 v = (rs1u >> (rs2u & 0x1f)) & 0xffffffff;
		m_register_file[instr.rd] = sign_extend<32>(v);
		break;
	}
	case Opcode::srliw: {
		u64 v = (rs1u >> (instr.imm & 0x1f)) & 0xffffffff;
		m_register_file[instr.rd] = sign_extend<32>(v);
		break;
	}
	case Opcode::sra: {
		m_register_file[instr.rd] = rs1 >> (rs2u & 0x3f);
		break;
	}
	case Opcode::srai: {
		m_register_file[instr.rd] = rs1 >> (instr.imm & 0x3f);
		break;
	}
	case Opcode::sraw: {
		u64 v = (rs1 >> (rs2u & 0x1f)) & 0xffffffff;
		m_register_file[instr.rd] = sign_extend<32>(v);
		break;
	}
	case Opcode::sraiw: {
		u64 v = (rs1 >> (instr.imm & 0x1f)) & 0xffffffff;
		m_register_file[instr.rd] = sign_extend<32>(v);
		break;
	}
	case Opcode::xor_: {
		m_register_file[instr.rd] = rs1u ^ rs2u;
		break;
	}
	case Opcode::or_: {
		m_register_file[instr.rd] = rs1u | rs2u;
		break;
	}
	case Opcode::and_: {
		m_register_file[instr.rd] = rs1u & rs2u;
		break;
	}
	case Opcode::xori: {
		m_register_file[instr.rd] = rs1u ^ static_cast<u64>(instr.imm);
		break;
	}
	case Opcode::ori: {
		m_register_file[instr.rd] = rs1u | static_cast<u64>(instr.imm);
		break;
	}
	case Opcode::andi: {
		m_register_file[instr.rd] = rs1u & static_cast<u64>(instr.imm);
		break;
	}
	case Opcode::sub: {
		m_register_file[instr.rd] = rs1u - rs2u;
		break;
	}
	case Opcode::subw: {
		u64 v = sign_extend<32>((rs1u - rs2u) & 0xffffffff);
		m_register_file[instr.rd] = v;
		break;
	}
	case Opcode::ecall: {
		u64 a0 = m_register_file[10];
		char const *str = reinterpret_cast<char const *>(m_memory.data() + a0);
		printf("ecall: %s\n", str);
		break;
	}
	case Opcode::ebreak:
		if (instr.size == 0)
		{
			size_t idx = static_cast<size_t>(instr.imm);
			m_next_instruction_override = &m_break_instructions[idx];
			return false;
		}
		break;

	// RV64M
	// mul
	// mulh
	// mulhsu
	// mulhu
	// div
	// divu
	// rem
	// remu
	// mulw
	// divw
	// divuw
	// remw
	// remuw

	// csrrw
	// csrrs
	// csrrc
	// csrrwi
	// csrrsi
	// csrrci

	default: break;
	}

	m_register_file[0] = 0;

	if (advance_pc) {
		m_pc += instr.size;
		m_ic += 1;
	}

	return true;
}

void *Standalone_Core::get_pc_ptr() {
	return &m_pc;
}

void *Standalone_Core::get_ic_ptr() {
	return &m_ic;
}

bool Standalone_Core::set_breakpoint(size_t instr_idx) {
	if (instr_idx >= m_instructions.size()) return false;

	for (size_t i = 0; i < m_break_instructions.size(); ++i) {
		Instruction &br_instr = m_break_instructions[i];
		if (br_instr.size == 0)
		{
			br_instr.imm = i;
			std::swap(m_instructions[instr_idx], br_instr);
			return true;
		}
	}

	return false;
}

bool Standalone_Core::clear_breakpoint(size_t instr_idx) {
	if (instr_idx >= m_instructions.size()) return false;

	Instruction &br_instr = m_instructions[instr_idx];
	if (br_instr.size == 0 && br_instr.opcode == Opcode::ebreak)
	{
		size_t break_idx = static_cast<size_t>(br_instr.imm);
		std::swap(br_instr, m_break_instructions[break_idx]);
		return true;
	}

	return false;
}

void Standalone_Core::reset() {
	m_register_file.fill(0);
	m_csrs.fill(0);
	m_memory.fill(0);

	m_pc = 0;
	m_ic = 0;
}

void Standalone_Core::run() {
	if (m_ic == m_instructions.size()) {
		reset();
	}
	while (execute_next_instruction());
}

void Standalone_Core::step() {
	bool is_break = !execute_next_instruction();
	if (is_break) execute_next_instruction();
}
