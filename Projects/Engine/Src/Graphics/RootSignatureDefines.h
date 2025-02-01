#pragma once

#include "Types.h"

namespace RS
{
	struct RootSignatureConstantsParam
	{
		uint32 DescCount;
	};

	struct RootSignatureTextureParam
	{
		uint32 DescCount;
		bool IsReadWrite;
	};

}
