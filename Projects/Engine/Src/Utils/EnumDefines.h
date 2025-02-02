#pragma once

#include "Utils/Misc/DylanForEachMacro.h"

/*
	Usage:

	RS_ENUM_FLAGS(uint, Type, A, B, C);

	Produces:
	enum Type : uint
	{
		A = 1,
		B = 2,
		C = 4,
		MASK = 7,
		NONE = 0,
		COUNT = 3
	};
*/

#define _RS_ENUM_FLAGS_ENTRY(x, i, d) inline static constexpr d x = (1 << (i));
#define RS_ENUM_FLAGS(type, name, ...) \
	typedef type name##s; \
	struct name \
	{ \
		RS_INDEXED_FOR_EACH_DATA(_RS_ENUM_FLAGS_ENTRY, type, __VA_ARGS__) \
		inline static constexpr type MASK = (1 << (RS_VA_NARGS(__VA_ARGS__)))-1; \
		inline static constexpr type COUNT = RS_VA_NARGS(__VA_ARGS__); \
		inline static constexpr type NONE = 0; \
	};

