#pragma once

#include "Graphics/Format.h"
#include "dxgiformat.h"

namespace RS
{
	static Format ToDXFormat(DXGI_FORMAT format);
}