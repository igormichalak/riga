struct Bit_Vector {
	std::vector<u64> bits;
	std::vector<u32> blocks;
	size_t num_bits;
};

void bv_build(Bit_Vector &bv);
u32 bv_rank(const Bit_Vector &bv, size_t pos);
