void bv_build(Bit_Vector &bv) {
	size_t n = bv.bits.size();
	bv.blocks.assign(n + 1, 0);
	for (size_t i = 0; i < n; ++i) {
		u32 ones = static_cast<u32>(std::popcount(bv.bits[i]));
		bv.blocks[i + 1] = bv.blocks[i] + ones;
	}
}

u32 bv_rank(const Bit_Vector &bv, size_t pos) {
	assert(pos <= bv.num_bits);

	size_t block_idx = pos >> 6;
	u32 offset = pos & 63;

	u32 result = bv.blocks[block_idx];

	if (offset) {
		u64 dw = bv.bits[block_idx];
		u64 mask = (1ULL << offset) - 1;
		result += static_cast<u32>(std::popcount(dw & mask));
	}

	return result;
}
