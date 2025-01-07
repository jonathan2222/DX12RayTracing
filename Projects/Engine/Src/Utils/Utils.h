#pragma once

#include "Misc/StringUtils.h"
#include "Misc/BitUtils.h"
#include "Misc/HashUtils.h"
#include "Misc/RandomUtils.h"
#include "Misc/MathUtils.h"
#include "Misc/ShapingFunctions.h"
#include <limits>

namespace RS::Utils
{
	// Numbers
	template<typename FType, typename IType>
	inline uint32 GetNumDecimals(FType v)
	{
		uint32 count = 0;
		FType num = std::abs(v);
		IType inum = IType(num);
		num = num - inum;
		if (num >= std::numeric_limits<IType>::max())
			return 0;
		while (num != 0)
		{
			num = num * 10;
			inum = IType(num);
			num = num - inum;
			count++;
		}
		return count;
	}
	inline uint32 GetNumDecimals(float v)
	{
		return GetNumDecimals<float, int64>(v);
	}
	inline uint32 GetNumDecimals(double v)
	{
		return GetNumDecimals<double, int64>(v);
	}

	inline bool AreValuesClose(float a, float b)
	{
		return std::abs(a - b) < FLT_EPSILON;
	}

}