#pragma once

#include <functional>
#include "Utils/Misc/BitUtils.h"

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

	// This requires SSE4.2 which is present on Intel Nehalem (Nov. 2008)
	// and AMD Bulldozer (Oct. 2011) processors.  I could put a runtime
	// check for this, but I'm just going to assume people playing with
	// DirectX 12 on Windows 10 have fairly recent machines.
	#ifdef _M_X64
	#define ENABLE_SSE_CRC32 1
	#else
	#define ENABLE_SSE_CRC32 0
	#endif

	#if ENABLE_SSE_CRC32
	#pragma intrinsic(_mm_crc32_u32)
	#pragma intrinsic(_mm_crc32_u64)
	#endif

	inline uint64 Hash64(const uint32* const pStart, const uint32* const pEnd, uint64 hash)
	{
#if ENABLE_SSE_CRC32
		const uint64* pStart64 = (const uint64*)AlignUp(pStart, 8);
		const uint64* pEnd64 = (const uint64*)AlignUp(pEnd, 8);

		// If not 64-bit aligned, start with a single u32
		if ((uint32_t*)pStart64 > pStart)
			hash = (uint64)_mm_crc32_u32((uint32)hash, *pStart);

		// Iterate over consecutive u64 values
		while (pStart64 < pEnd64)
			hash = (uint64)_mm_crc32_u64((uint64)hash, *pStart64++);

		// If there is a 32-bit remainder, accumulate that
		if ((uint32*)pStart64 < pEnd)
			hash = (uint64)_mm_crc32_u32((uint32)hash, *(uint32_t*)pStart64);
#else
		// An inexpensive hash for CPUs lacking SSE4.2
		for (const uint32* pIter = pStart; pIter < pEnd; ++pIter)
			hash = 16777619U * hash ^ *pIter;
#endif
		return hash;
	}

	template <typename T>
	inline uint64 Hash64State(const T* pStateDesc, size_t count = 1, uint64 hash = 2166136261U)
	{
		static_assert((sizeof(T) & 3) == 0 && alignof(T) >= 4, "State object is not word-aligned");
		return Hash64((uint32*)pStateDesc, (uint32*)(pStateDesc + count), hash);
	}

	inline uint64 Hash64(const std::string& str, uint64 hash = 2166136261U)
	{
		std::string wordAligned(str);
		if ((wordAligned.size() & 3) == 0)
		{
			const uint64 bytesToAdd = wordAligned.size() & (~3);
			wordAligned.insert(wordAligned.end(), bytesToAdd, ' ');
		}
		return Hash64((uint32*)wordAligned.data(), (uint32*)(wordAligned.data() + wordAligned.size()), hash);
	}

	template <typename T>
	inline uint64 Hash64(const T& value, uint64 hash = 2166136261U)
	{
		uint64 finalValue;
		std::memcpy((void*)&finalValue, (void*)&value, std::min(sizeof(value), sizeof(uint64)));
		return Hash64((uint32*)&finalValue, (uint32*)(&finalValue + 1), hash);
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