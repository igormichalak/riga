#include <cinttypes>
#include <cstddef>

using std::size_t;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using s8 = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

template <unsigned N>
constexpr s64 sign_extend(u32 w) {
	static_assert(N > 0 && N <= 32, "invalid bit-width");
	const u64 m = 1U << (N - 1);
	return static_cast<s64>((static_cast<u64>(w) ^ m) - m);
}

enum class Decode_Error {
	Illegal_Instruction
};
