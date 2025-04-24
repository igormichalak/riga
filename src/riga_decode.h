enum class Instr_Fmt : u8 {
	U, J, I, B, S, R, Unknown
};

inline Instr_Fmt decode_instr_fmt(u32 w);

std::string_view to_string(Instr_Fmt fmt) noexcept;
