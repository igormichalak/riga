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

class Standalone_Core {
public:
	static constexpr size_t MEMORY_SIZE = 1024 * 1024;

	void program_compile();
	void link_flash_memory(void *p, size_t len);
private:
	// x0-x31 registers.
	std::array<u64, 32> m_register_file{};

	// The only 4 CSRs accessible from within the user-mode.
	// (0) 0xC00 - cycle
	// (1) 0xC01 - time
	// (2) 0xC02 - instret
	// (3) 0xC03 - hpmcounter3
	std::array<u64, 4> m_csrs{};

	std::array<u8, MEMORY_SIZE> m_memory{};

	u64                      m_pc{0};
	std::vector<Instruction> m_instructions;
	Bit_Vector               m_instruction_offset_vec;

	// Variably-sized flash/program memory.
	u8    *m_flash_memory{nullptr};
	size_t m_flash_memory_size{0};

	auto load_instruction(u32 w) -> std::expected<size_t, Decode_Error>;
};

#include "glue.cpp"

