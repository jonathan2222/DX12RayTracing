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

#define _RS_ENUM_FLAGS_ENTRY(x, i) x = (1 << (i)),
#define RS_ENUM_FLAGS(type, name, ...) \
	enum name : type \
	{ \
		RS_INDEXED_FOR_EACH(_RS_ENUM_FLAGS_ENTRY, __VA_ARGS__) \
		MASK = (1 << (RS_VA_NARGS(__VA_ARGS__)))-1, \
		COUNT = RS_VA_NARGS(__VA_ARGS__), \
		NONE = 0 \
	}

