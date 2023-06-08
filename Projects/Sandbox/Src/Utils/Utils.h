#pragma once

#include "Misc/StringUtils.h"
#include "Misc/BitUtils.h"
#include "Misc/HashUtils.h"

namespace RS::Utils
{
	// Numbers
	template<typename FType, typename IType>
	inline uint32 GetNumDecimals(FType v)
	{
		uint32 count = 0;
		FType num = std::abs(v);
		num = num - IType(num);
		while (num != 0)
		{
			num = num * 10;
			num = num - IType(num);
			count++;
		}
		return count;
	}
	inline uint32 GetNumDecimals(float v)
	{
		return GetNumDecimals<float, int32>(v);
	}
	inline uint32 GetNumDecimals(double v)
	{
		return GetNumDecimals<double, int64>(v);
	}

}