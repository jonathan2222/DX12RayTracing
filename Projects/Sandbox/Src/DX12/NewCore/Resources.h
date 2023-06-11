#pragma once

#include "DX12/NewCore/Resource.h"

namespace RS
{
	class Buffer : public Resource
	{
	public:
		Buffer();
		virtual ~Buffer();

	private:
	};

	class Texture : public Resource
	{
	public:
		Texture() : Resource() {}
		virtual ~Texture() {}

	private:
	};
}