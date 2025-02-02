#pragma once

// From: Dylan's answer https://stackoverflow.com/questions/14732803/preprocessor-variadic-for-each-macro-compatible-with-msvc10
// Modified to also include an index
// Count from the internet :D

#ifndef RS_CROSS_PLATFORM_PREPROCESSOR
	error "Cannot use RS_VA_COUNT with the tranditional preprocessor!"
#endif

/*
	Usage:
	RS_VA_NARGS(A, B, C)

	Output:
	3
*/
#define RS_VA_NARGS(...) RS_VA_NARGS_IMPL(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#define RS_VA_COUNT(...) RS_VA_COUNT_N(__VA_ARGS__)

/*
	Usage:
	#define Foo(x) int x = 0;
	RS_FOR_EACH(Foo, A, B, C)

	Output:
	int A = 0;
	int B = 0;
	int C = 0;
*/
#define RS_FOR_EACH(what, ...) RS_FOR_EACH_N(RS_VA_NARGS(__VA_ARGS__), what, __VA_ARGS__)

/*
	Usage:
	#define Foo(x, i) int x = i;
	RS_INDEXED_FOR_EACH(Foo, A, B, C)

	Output:
	int A = 0;
	int B = 1;
	int C = 2;
*/
#define RS_INDEXED_FOR_EACH(what, ...) RS_INDEXED_FOR_EACH_N(RS_VA_NARGS(__VA_ARGS__), what, __VA_ARGS__)


/*
	Usage:
	#define Foo(x, i, d) d x = i;
	RS_INDEXED_FOR_EACH_DATA(Foo, int, A, B, C)

	Output:
	int A = 0;
	int B = 1;
	int C = 2;
*/
#define RS_INDEXED_FOR_EACH_DATA(what, data, ...) RS_INDEXED_FOR_EACH_DATA_N(RS_VA_NARGS(__VA_ARGS__), what, data, __VA_ARGS__)

// Implementation of macro argument count
#define RS_VA_NARGS_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N

// Misc macros
#define RS_EXPAND(x) x
#define RS_CONCATENATE(x,y) x##y

// Implementation of RS_FOR_EACH
#define RS_FOR_EACH_1(what, x, ...)  what(x)
#define RS_FOR_EACH_2(what, x, ...)  what(x)RS_EXPAND(RS_FOR_EACH_1(what,  __VA_ARGS__))
#define RS_FOR_EACH_3(what, x, ...)  what(x)RS_EXPAND(RS_FOR_EACH_2(what,  __VA_ARGS__))
#define RS_FOR_EACH_4(what, x, ...)  what(x)RS_EXPAND(RS_FOR_EACH_3(what,  __VA_ARGS__))
#define RS_FOR_EACH_5(what, x, ...)  what(x)RS_EXPAND(RS_FOR_EACH_4(what,  __VA_ARGS__))
#define RS_FOR_EACH_6(what, x, ...)  what(x)RS_EXPAND(RS_FOR_EACH_5(what,  __VA_ARGS__))
#define RS_FOR_EACH_7(what, x, ...)  what(x)RS_EXPAND(RS_FOR_EACH_6(what,  __VA_ARGS__))
#define RS_FOR_EACH_8(what, x, ...)  what(x)RS_EXPAND(RS_FOR_EACH_7(what,  __VA_ARGS__))
#define RS_FOR_EACH_9(what, x, ...)  what(x)RS_EXPAND(RS_FOR_EACH_8(what,  __VA_ARGS__))
#define RS_FOR_EACH_10(what, x, ...) what(x)RS_EXPAND(RS_FOR_EACH_9(what,  __VA_ARGS__))
#define RS_FOR_EACH_N(N, what, ...) RS_EXPAND(RS_CONCATENATE(RS_FOR_EACH_, N)(what, __VA_ARGS__))

// Implementation of RS_INDEXED_FOR_EACH
#define RS_INDEXED_FOR_EACH_1(what, index, x, ...) what(x, index)
#define RS_INDEXED_FOR_EACH_2(what, index, x, ...) what(x, index)RS_EXPAND(RS_INDEXED_FOR_EACH_1(what, index+1, __VA_ARGS__))
#define RS_INDEXED_FOR_EACH_3(what, index, x, ...) what(x, index)RS_EXPAND(RS_INDEXED_FOR_EACH_2(what, index+1, __VA_ARGS__))
#define RS_INDEXED_FOR_EACH_4(what, index, x, ...) what(x, index)RS_EXPAND(RS_INDEXED_FOR_EACH_3(what, index+1, __VA_ARGS__))
#define RS_INDEXED_FOR_EACH_5(what, index, x, ...) what(x, index)RS_EXPAND(RS_INDEXED_FOR_EACH_4(what, index+1, __VA_ARGS__))
#define RS_INDEXED_FOR_EACH_6(what, index, x, ...) what(x, index)RS_EXPAND(RS_INDEXED_FOR_EACH_5(what, index+1, __VA_ARGS__))
#define RS_INDEXED_FOR_EACH_7(what, index, x, ...) what(x, index)RS_EXPAND(RS_INDEXED_FOR_EACH_6(what, index+1, __VA_ARGS__))
#define RS_INDEXED_FOR_EACH_8(what, index, x, ...) what(x, index)RS_EXPAND(RS_INDEXED_FOR_EACH_7(what, index+1, __VA_ARGS__))
#define RS_INDEXED_FOR_EACH_9(what, index, x, ...) what(x, index)RS_EXPAND(RS_INDEXED_FOR_EACH_8(what, index+1, __VA_ARGS__))
#define RS_INDEXED_FOR_EACH_10(what, index, x, ...) what(x,index)RS_EXPAND(RS_INDEXED_FOR_EACH_9(what, index+1, __VA_ARGS__))
#define RS_INDEXED_FOR_EACH_N(N, what, ...) RS_EXPAND(RS_CONCATENATE(RS_INDEXED_FOR_EACH_, N)(what, 0, __VA_ARGS__))

// Implementation of RS_INDEXED_FOR_EACH_DATA
#define RS_INDEXED_FOR_EACH_DATA_1(what, index, data, x, ...) what(x, index, data)
#define RS_INDEXED_FOR_EACH_DATA_2(what, index, data, x, ...) what(x, index, data)RS_EXPAND(RS_INDEXED_FOR_EACH_DATA_1(what, index+1, data, __VA_ARGS__))
#define RS_INDEXED_FOR_EACH_DATA_3(what, index, data, x, ...) what(x, index, data)RS_EXPAND(RS_INDEXED_FOR_EACH_DATA_2(what, index+1, data, __VA_ARGS__))
#define RS_INDEXED_FOR_EACH_DATA_4(what, index, data, x, ...) what(x, index, data)RS_EXPAND(RS_INDEXED_FOR_EACH_DATA_3(what, index+1, data, __VA_ARGS__))
#define RS_INDEXED_FOR_EACH_DATA_5(what, index, data, x, ...) what(x, index, data)RS_EXPAND(RS_INDEXED_FOR_EACH_DATA_4(what, index+1, data, __VA_ARGS__))
#define RS_INDEXED_FOR_EACH_DATA_6(what, index, data, x, ...) what(x, index, data)RS_EXPAND(RS_INDEXED_FOR_EACH_DATA_5(what, index+1, data, __VA_ARGS__))
#define RS_INDEXED_FOR_EACH_DATA_7(what, index, data, x, ...) what(x, index, data)RS_EXPAND(RS_INDEXED_FOR_EACH_DATA_6(what, index+1, data, __VA_ARGS__))
#define RS_INDEXED_FOR_EACH_DATA_8(what, index, data, x, ...) what(x, index, data)RS_EXPAND(RS_INDEXED_FOR_EACH_DATA_7(what, index+1, data, __VA_ARGS__))
#define RS_INDEXED_FOR_EACH_DATA_9(what, index, data, x, ...) what(x, index, data)RS_EXPAND(RS_INDEXED_FOR_EACH_DATA_8(what, index+1, data, __VA_ARGS__))
#define RS_INDEXED_FOR_EACH_DATA_10(what, index, data, x, ...) what(x,index, data)RS_EXPAND(RS_INDEXED_FOR_EACH_DATA_9(what, index+1, data, __VA_ARGS__))
#define RS_INDEXED_FOR_EACH_DATA_N(N, what, data, ...) RS_EXPAND(RS_CONCATENATE(RS_INDEXED_FOR_EACH_DATA_, N)(what, 0, data, __VA_ARGS__))
