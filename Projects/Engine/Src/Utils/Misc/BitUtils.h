#pragma once

namespace RS::Utils
{
	template<typename T>
	inline T AlignUp(T x, T alignment)
	{
		// Align 1010 1101b with 4 (100b)
		// mask 4 or greater => 4-1 = 11b
		// invert mask => 1111 1100b
		// If and with x we get the aligned value rounded down.
		// 1010 1101
		// 1111 1100 &
		// 1010 1100 (lost 1 bit)
		// Adding one less of the alignment (3=11b) we get the aligned value rounded up.
		// Added 3:
		// 1010 1101
		// 0000 0011 +
		// 1011 0000
		// New aligned value:
		// 1011 0000
		// 1111 1100 &
		// 1011 0000
		return (x + (alignment - 1)) & ~(alignment - 1);
	}

	template<typename T>
	inline T AlignDown(T x, T alignment)
	{
		return x & ~(alignment - 1);
	}

	// Check if power of 2 and treat 0 as false. (with other words, check if only one bit is set)
	template<typename T>
	inline bool IsPowerOfTwo(T x)
	{
		return x && !(x & (x - 1));
	}

	template<typename T>
	inline T IndexOfPow2Bit(T x)
	{
		if (!IsPowerOfTwo<T>(x))
			return (T)-1;

		T i = 1, pos = 0;
		while (!(i & x))
		{
			i = i << 1;
			++pos;
		}
		return pos;
	}

	template<typename T>
	inline bool IsBitSet(T v, T bit)
	{
		return v & (1 << bit);
	}
}