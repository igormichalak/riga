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

class Standalone_Core {
public:
	void init();

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

	bool set_breakpoint(size_t instr_idx);
	bool clear_breakpoint(size_t instr_idx);

	void reset();
	void run();
	void step();
private:
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

	Instruction *m_next_instruction_override = nullptr;

	auto load_instruction(u32 w) -> std::expected<size_t, Decode_Error>;
	size_t calculate_ic(u64 pc);
	bool execute_next_instruction();
};

#include "glue.cpp"

