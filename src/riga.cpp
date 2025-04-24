#include "riga.h"

#include <cassert>
#include <expected>
#include <array>
#include <vector>
#include <bit>
#include <iostream>
#include <format>

#include "riga_bit_vector.h"
#include "riga_decode.h"
#include "riga_opcode.h"

#include "riga_bit_vector.cpp"
#include "riga_decode.cpp"

struct Instruction {
	Opcode opcode;
	union {
		struct {
			// U,J,I,R-type -> rd
			// B,S-type -> rs2
			u8 op0;
			// U,J-type -> (4 imm LSB) << 4
			// I,B,S,R-type -> rs1/zimm
			u8 op1;
			u16 imm;
		};
		u32 opbits;
	};
};

enum class Decode_Error {};

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

	u64                      m_pc{0};
	std::vector<Instruction> m_instructions;
	Bit_Vector               m_instruction_offset_vec;

	// Variably-sized flash/program memory.
	u8    *m_flash_memory{nullptr};
	size_t m_flash_memory_size{0};

	auto load_instruction(u32 w) -> std::expected<size_t, Decode_Error>;

	void program_compile();
	void link_flash_memory(void *p, size_t len);
};


auto Standalone_Core::load_instruction(u32 w)
	-> std::expected<size_t, Decode_Error>
{
	Instr_Fmt ifmt = decode_instr_fmt(w);
	std::cout << std::format("{} {:#010x}\n", to_string(ifmt), w);
	return 4;
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
	m_flash_memory      = static_cast<u8 *>(p);
	m_flash_memory_size = len;
}
