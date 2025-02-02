#pragma once

#include "Types.h"

namespace RS::Utils
{
	template <typename T>
	inline T AlignUpWithMask(T value, uint64 mask)
	{
		return (T)(((uint64)value + mask) & ~mask);
	}

	template <typename T>
	inline T AlignDownWithMask(T value, uint64 mask)
	{
		return (T)((uint64)value & ~mask);
	}

	template<typename T>
	inline T AlignUp(T x, uint64 alignment)
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
		//return (x + (alignment - 1)) & ~(alignment - 1);
		return AlignUpWithMask<T>(x, alignment - 1);
	}

	template<typename T>
	inline T AlignDown(T x, uint64 alignment)
	{
		//return x & ~(alignment - 1);
		return AlignDownWithMask<T>(x, alignment - 1);
	}

	template <typename T>
	inline bool IsAligned(T value, uint64 alignment)
	{
		return 0 == ((uint64)value & (alignment - 1));
	}

	template <typename T>
	inline T DivideByMultiple(T value, uint64 alignment)
	{
		return (T)((value + alignment - 1) / alignment);
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

	template<typename T>
	inline void SetBit(T& mask, T bit)
	{
		mask = mask | (1 << bit);
	}

	template<typename T>
	inline void ClearBit(T& mask, T bit)
	{
		mask = mask & (~(1 << bit));
	}

	// index is the index of the first set bit from LSB to MSB.
	// If no bit is set, it will have the value UINT32_MAX.
	// The function returns true if the mask is non-zero else false.
	inline bool ForwardBitScan(uint32 mask, uint32& index)
	{
		unsigned long i;
		char isNonZero = _BitScanForward(&i, mask);
		index = isNonZero ? (uint32)i : UINT32_MAX;
		return isNonZero != 0;
	}

	// index is the index of the first set bit from LSB to MSB.
	// If no bit is set, it will have the value UINT32_MAX.
	// The function returns true if the mask is non-zero else false.
	inline bool ForwardBitScan(uint64 mask, uint32& index)
	{
		unsigned long i;
		char isNonZero = _BitScanForward64(&i, mask);
		index = isNonZero ? (uint32)i : UINT32_MAX;
		return isNonZero != 0;
	}

	// index is the index of the first set bit from MSB to LSB.
	// If no bit is set, it will have the value UINT32_MAX.
	// The function returns true if the mask is non-zero else false.
	inline bool ReverseBitScan(uint32 mask, uint32& index)
	{
		unsigned long i;
		char isNonZero = _BitScanReverse(&i, mask);
		index = isNonZero ? (uint32)i : UINT32_MAX;
		return isNonZero != 0;
	}

	// Returns the index of the first set bit from MSB to LSB.
	// If no bit is set, it will return UINT32_MAX.
	inline bool ReverseBitScan(uint64 mask, uint32& index)
	{
		unsigned long i;
		char isNonZero = _BitScanReverse64(&i, mask);
		index = isNonZero ? (uint32)i : UINT32_MAX;
		return isNonZero != 0;
	}
}