#pragma once

#include <functional>

namespace RS::Utils
{
	inline uint64 Hash64(uint64 v)
	{
		// Murmur64 hash
		v ^= v >> 33;
		v *= 0xff51afd7ed558ccdL;
		v ^= v >> 33;
		v *= 0xc4ceb9fe1a85ec53L;
		v ^= v >> 33;
		return v;
	}

	inline uint64 CombineHash64(uint64 a, uint64 b)
	{
		// From: https://stackoverflow.com/questions/76076957/fastest-way-to-bring-a-range-from-to-of-64-bit-integers-into-pseudo-random-or
		auto x = a + b;
		x ^= x >> 12;
		x ^= x << 25;
		x ^= x >> 27;
		return x * 0x2545f4914f6cdd1dull;
	}

	template<typename T>
	inline uint64 Hash(const T& v)
	{
		return (uint64)std::hash<T>{}(v);
	}

	template<typename T1, typename T2>
	inline uint64 Hash(const T1& a, const T2& b)
	{
		uint64 hashA = Hash<T1>(a);
		uint64 hashB = Hash<T2>(b);
		return CombineHash64(hashA, hashB);
	}
}