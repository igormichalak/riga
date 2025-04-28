enum class Instr_Fmt : u8 {
	U, J, I, B, S, R, Unknown
};

inline Instr_Fmt decode_instr_fmt(u32 w);

std::string_view to_string(Instr_Fmt fmt) noexcept;

std::string_view to_reg_name(u8 reg) noexcept;

inline u8 opcode (u32 w);
inline u8 funct3 (u32 w);
inline u8 funct7 (u32 w);
inline u8 rd     (u32 w);
inline u8 rs1    (u32 w);
inline u8 rs2    (u32 w);

inline s64 imm_i(u32 w);
inline s64 imm_s(u32 w);
inline s64 imm_b(u32 w);
inline s64 imm_u(u32 w);
inline s64 imm_j(u32 w);

auto decode_opcode(u32 w) -> std::expected<Opcode, Decode_Error>;
