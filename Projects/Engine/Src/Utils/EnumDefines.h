#pragma once

#include "Utils/Misc/DylanForEachMacro.h"
#include "Utils/Misc/BitUtils.h"

/*
	Usage:

	RS_ENUM_FLAGS(uint, Type, A, B, C);

	Produces:
	using Types = uint;
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
//#define RS_ENUM_FLAGS(type, name, ...) \
//	using name##s = type; \
//	enum class name : type \
//	{ \
//		RS_INDEXED_FOR_EACH_DATA(_RS_ENUM_FLAGS_ENTRY, type, __VA_ARGS__) \
//		MASK = (1 << (RS_VA_NARGS(__VA_ARGS__)))-1, \
//		COUNT = RS_VA_NARGS(__VA_ARGS__), \
//		NONE = 0 \
//	};
//#define _RS_ENUM_FLAGS_ENTRY(x, i, d) x = (1 << (i)),

/*
	Usage:

	RS_STRUCT_FLAGS(uint, Type, A, B, C);

	Produces something similar to:
	using Types = Type;
	enum Type : uint
	{
		A = 1,
		B = 2,
		C = 4,
		MASK = 7,
		NONE = 0,
		COUNT = 3
	};

	Where this is possible:

	Types flags = Type::NONE;
	uint flagsValue = Type::NONE;
	Type flags2 = Type::NONE;
	const char* flagsName = flags.GetName();
	const char* flags2Name = Type::GetName(flags);
	flags.Has(Type::A)
	flags.Set(Type::B)
	flags.Clear(Type::C)
	flags.IsOne() -> true if only one flag is set.
	flags.Get() -> returns the reference to the underlying value.
*/
// TODO: Move these utils functions outside the class to make the declaration less intense!
#define _RS_ENUM_FLAGS_ENTRY(x, i, d) inline static constexpr d x = (d)(1 << (i));
#define _RS_ENUM_FLAGS_ENTRY_STR(x) #x, 
#define _RS_ENUM_FLAGS_ENTRY_STR_END(x) #x
#define RS_ENUM_FLAGS(type, name, ...) \
	struct name \
	{ \
		RS_INDEXED_FOR_EACH_DATA(_RS_ENUM_FLAGS_ENTRY, type, __VA_ARGS__) \
		inline static constexpr type MASK = (type)((1 << (RS_VA_NARGS(__VA_ARGS__)))-1); \
		inline static constexpr type NONE = (type)0; \
		inline static constexpr type COUNT = (type)(RS_VA_NARGS(__VA_ARGS__)); \
		\
		name() : m_Value((type)0) {} \
		constexpr name(type value) : m_Value(value) {} \
		constexpr name(const name& other) : m_Value(other.m_Value) {} \
		name(name&& other) : m_Value(other.m_Value) {} \
		operator type() { return m_Value; } \
		static const char* GetName(const name& flag) \
		{ \
			if (flag == NONE) return ms_pStrings[0]; \
			if (flag.m_Value >= (1 << (COUNT-1)) || !RS::Utils::IsPowerOfTwo(flag.m_Value)) return ms_pStrings[COUNT+1]; \
			uint32 index; \
			RS::Utils::ForwardBitScan(flag.m_Value, index); \
			return ms_pStrings[index]; \
		} \
		const char* GetName() const { return GetName(*this); } \
		bool Has(const name& flags) const { return (m_Value & flags.m_Value) == flags.m_Value; } \
		void Set(const name& flags) { m_Value |= flags.m_Value; } \
		void Clear(const name& flags) { m_Value = (m_Value & (~flags.m_Value)); } \
		bool IsOne() const { return RS::Utils::IsPowerOfTwo(m_Value); } \
		type& Get() { return m_Value; } \
		type GetConst() const { return m_Value; } \
		\
		template<NumberType T> \
		name operator&(T other) const { return name(m_Value & (type)other);} \
		name operator&(const name& other) const { return name(m_Value & other.m_Value);} \
		template<NumberType T> \
		name operator|(T other) const { return name(m_Value | (type)other);} \
		name operator|(const name& other) const { return name(m_Value | other.m_Value);} \
		template<NumberType T> \
		name operator^(T other) const { return name(m_Value ^ (type)other);} \
		name operator^(const name& other) const { return name(m_Value ^ other.m_Value);} \
		template<NumberType T> \
		name& operator=(T other) { m_Value = (type)other; return *this; } \
		name& operator=(const name& other) { m_Value = other.m_Value; return *this; } \
		template<NumberType T> \
		name& operator&=(T other) { m_Value = m_Value & (type)other; return *this; } \
		name& operator&=(const name& other) { m_Value = m_Value & other.m_Value; return *this; } \
		template<NumberType T> \
		name& operator^=(T other) { m_Value = m_Value ^ (type)other; return *this; } \
		name& operator^=(const name& other) { m_Value = m_Value ^ other.m_Value; return *this; } \
		template<NumberType T> \
		name& operator|=(T other) { m_Value = m_Value | (type)other; return *this; } \
		name& operator|=(const name& other) { m_Value = m_Value | other.m_Value; return *this; } \
		template<NumberType T> \
		bool operator==(T other) const { return m_Value == (type)other; } \
		bool operator==(const name& other) const { return m_Value == other.m_Value; } \
		template<NumberType T> \
		bool operator>(T other) const { return m_Value > (type)other; } \
		bool operator>(const name& other) const { return m_Value > other.m_Value; } \
		template<NumberType T> \
		bool operator<(T other) const { return m_Value < (type)other; } \
		bool operator<(const name& other) const { return m_Value < other.m_Value; } \
		template<NumberType T> \
		bool operator>=(T other) const { return m_Value >= (type)other; } \
		bool operator>=(const name& other) const { return m_Value >= other.m_Value; } \
		template<NumberType T> \
		bool operator<=(T other) const { return m_Value <= (type)other; } \
		bool operator<=(const name& other) const { return m_Value <= other.m_Value; } \
		template<NumberType T> \
		name operator>>(T other) const { return name(m_Value >> (type)other); } \
		name operator>>(const name& other) const { return name(m_Value >> other.m_Value); } \
		template<NumberType T> \
		name operator<<(T other) const { return name(m_Value << (type)other); } \
		name operator<<(const name& other) const {return name(m_Value << other.m_Value); } \
		name& operator++() { ++m_Value; return *this; } \
		name operator++(int) { name ret(m_Value); ++m_Value; return ret; } \
	private: \
		static inline const char* ms_pStrings[COUNT+2] = { "NONE", RS_FOR_EACH_END(_RS_ENUM_FLAGS_ENTRY_STR, _RS_ENUM_FLAGS_ENTRY_STR_END, __VA_ARGS__), "?" }; \
		type m_Value; \
	}; \
	using name##s = name;
