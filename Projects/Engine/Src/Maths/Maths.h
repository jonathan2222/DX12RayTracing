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

		template<FloatType Type>
		inline static Type Saturate(Type value)
		{
			if (value < (Type)0) return (Type)0;
			else if (value > (Type)1) return (Type)1;
			else return value;
		};
	}
}
