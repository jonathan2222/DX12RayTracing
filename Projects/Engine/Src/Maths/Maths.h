#pragma once

namespace RS
{
	namespace Maths
	{
		template<NumberType Type>
		inline static Type Abs(Type value)
		{
			return value < (Type)0 ? (Type)-value : value;
		}
	}
}
