consteval bool wc_match(u8 field, const char pattern[6]) {
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
