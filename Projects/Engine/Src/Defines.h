#pragma once

#include <cassert>
#include <vector>
#include <string>

#ifndef RS_PROJECT_NAME
#define RS_PROJECT_NAME "RSDefault"
#endif

#define RS_UNUSED(x) ((void*)x)

#define LOG_DEBUG_ENABLED 1
#define LOG_ENABLE_WINDOWS_DEBUGGER_LOGGING 1
#define LOG_FLUSH_INTERVAL 2 // In seconds

#define FRAME_BUFFER_COUNT 3

#if !defined(_MSVC_TRADITIONAL) || _MSVC_TRADITIONAL
// Logic using the traditional preprocessor
#define RS_TRADITIONAL_PREPROCESSOR
#else
// Logic using cross-platform compatible preprocessor
#define RS_CROSS_PLATFORM_PREPROCESSOR
#endif

#include "Utils/EnumDefines.h"

// Concepts
template <typename T>
concept NumberType = std::unsigned_integral<T> || std::integral<T> || std::floating_point<T>;
template <typename T>
concept FloatType = std::floating_point<T>;
template<typename T>
concept ForceUIntType = std::unsigned_integral<T>;
template<typename T>
concept VecSizeConstant = ForceUIntType<T> && requires (T t) {
	{ t > 0 } -> std::convertible_to<bool>;
};

#define RS_CONFIG_DEVELOPMENT defined(RS_CONFIG_DEBUG) || defined(RS_CONFIG_RELEASE)
#include "Utils/Logger.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#ifdef RS_THROW_INSTEAD_OF_ASSERT
	#include <stdexcept>
	#include <format>
	inline std::string RS_ASSERT_INTERNAL_GetString()
	{
		return "Error";
	}
	template<typename... Args>
	inline std::string RS_ASSERT_INTERNAL_GetString(const char* formatStr, Args&&... arguments)
	{
		return std::vformat(formatStr, std::make_format_args(arguments...));
	}
	template<typename T>
	inline std::string RS_ASSERT_INTERNAL_GetString(const T& msg)
	{
		return msg;
	}
	#define RS_INTERNAL_ASSERT(exp, ...) { if (!(exp)) throw std::runtime_error(RS_ASSERT_INTERNAL_GetString(__VA_ARGS__)); }
	#define RS_INTERNAL_ASSERT_ALWAYS(...) { throw std::runtime_error(RS_ASSERT_INTERNAL_GetString(__VA_ARGS__)); }
#elif defined(RS_NO_ASSERT)
	#define RS_INTERNAL_ASSERT(exp, ...)
	#define RS_INTERNAL_ASSERT_ALWAYS(...)
#else
	#ifdef RS_CONFIG_DEVELOPMENT
		inline void RS_ASSERT_INTERNAL_LogCritical(const char* pFileName, int line, const char* pFuncName)
		{
			RS_UNUSED(pFileName);
			RS_UNUSED(line);
			RS_UNUSED(pFuncName);
		}
		template<typename... Args>
		inline void RS_ASSERT_INTERNAL_LogCritical(const char* pFileName, int line, const char* pFuncName, std::format_string<Args...> fmt, Args&&... arguments)
		{
			RS::Logger::GetMultiLogger()->log(spdlog::source_loc{ pFileName, line, pFuncName }, spdlog::level::critical, fmt, std::forward<Args>(arguments)...);
			LOG_FLUSH();
		}
		template<typename T>
		inline void RS_ASSERT_INTERNAL_LogCritical(const char* pFileName, int line, const char* pFuncName, const T& msg)
		{
			RS::Logger::GetMultiLogger()->log(spdlog::source_loc{ pFileName, line, pFuncName }, spdlog::level::critical, msg);
			LOG_FLUSH();
		}
		#define RS_INTERNAL_LOG_CRITICAL(...)
		#ifdef RS_CROSS_PLATFORM_PREPROCESSOR
			#define _RS_ASSERT_INTERNAL_LogCritical(file, line, ...) RS_ASSERT_INTERNAL_LogCritical(file, line, __VA_ARGS__)
			#define RS_INTERNAL_ASSERT(exp, ...) {if(!(exp)){ _RS_ASSERT_INTERNAL_LogCritical(__FILE__, __LINE__, SPDLOG_FUNCTION, ## __VA_ARGS__); } assert(exp); }
			#define RS_INTERNAL_ASSERT_ALWAYS(...) { _RS_ASSERT_INTERNAL_LogCritical(__FILE__, __LINE__, SPDLOG_FUNCTION, ## __VA_ARGS__); assert(false); }
		#else
			#define RS_INTERNAL_ASSERT(exp, ...) {if(!(exp)){ RS_ASSERT_INTERNAL_LogCritical(__FILE__, __LINE__, SPDLOG_FUNCTION, __VA_ARGS__); } assert(exp); }
			#define RS_INTERNAL_ASSERT_ALWAYS(...) { RS_ASSERT_INTERNAL_LogCritical(__FILE__, __LINE__, SPDLOG_FUNCTION, __VA_ARGS__); assert(false); }
		#endif	
	#else
		#define RS_INTERNAL_ASSERT(exp, ...) {assert(exp);}
		#define RS_INTERNAL_ASSERT_ALWAYS(...) {assert(false);}
	#endif
#endif

#ifdef RS_CROSS_PLATFORM_PREPROCESSOR
	#define RS_ASSERT(exp, ...) RS_INTERNAL_ASSERT(exp, ## __VA_ARGS__)
	#define RS_ASSERT_ALWAYS(...) RS_INTERNAL_ASSERT_ALWAYS(__VA_ARGS__)
#else
	#define RS_ASSERT(exp, ...) RS_INTERNAL_ASSERT(exp, __VA_ARGS__)
	#define RS_ASSERT_ALWAYS(...) RS_INTERNAL_ASSERT_ALWAYS(__VA_ARGS__)
#endif

#ifdef RS_CONFIG_DEVELOPMENT
	#define RS_DEBUG(code) code
#else
	#define RS_DEBUG(code)
#endif

#ifdef RS_CONFIG_DEVELOPMENT
	inline void RS_LOG_WARNING_ONCE_IF_LogWarning(const char* pFileName, int line, const char* pFuncName)
	{
		RS_UNUSED(pFileName);
		RS_UNUSED(line);
		RS_UNUSED(pFuncName);
	}
	template<typename... Args>
	inline void RS_LOG_WARNING_ONCE_IF_LogWarning(const char* pFileName, int line, const char* pFuncName, std::format_string<Args...> fmt, Args&&... arguments)
	{
		RS::Logger::GetMultiLogger()->log(spdlog::source_loc{ pFileName, line, pFuncName }, spdlog::level::warn, fmt, std::forward<Args>(arguments)...);
	}
	template<typename T>
	inline void RS_LOG_WARNING_ONCE_IF_LogWarning(const char* pFileName, int line, const char* pFuncName, const T& msg)
	{
		RS::Logger::GetMultiLogger()->log(spdlog::source_loc{ pFileName, line, pFuncName }, spdlog::level::warn, msg);
	}
	#define _RS_LOG_WARNING_ONCE_IF_LogWarning(file, line, ...) RS_LOG_WARNING_ONCE_IF_LogWarning(file, line, __VA_ARGS__)

	#define RS_LOG_WARNING_ONCE_IF(isTrue, ...) \
	{ \
		static bool sTriggeredWarning = false; \
		if ((bool)(isTrue) && !sTriggeredWarning) { \
			sTriggeredWarning = true; \
			RS_LOG_WARNING_ONCE_IF_LogWarning(__FILE__, __LINE__, SPDLOG_FUNCTION, ## __VA_ARGS__); \
		}\
	}
	#define RS_LOG_WARNING_ONCE_IF_NOT(isTrue, ...) RS_LOG_WARNING_ONCE_IF(!(isTrue), __VA_ARGS__)
#else
	#define RS_LOG_WARNING_ONCE_IF(isTrue, ...) 
	#define RS_LOG_WARNING_ONCE_IF_NOT(isTrue, ...)
#endif

#define RS_CONFIG_FILE_PATH "Config/EngineConfig.cfg"
#define RS_SHADER_PATH "Shaders/"
#define RS_TEXTURE_PATH "Textures/"
#define RS_MODEL_PATH "Models/"
#define RS_FONT_PATH "Fonts/"
#define RS_AUDIO_PATH "Audio/"

#define RS_UNREFERENCED_VARIABLE(v) (void)v
#define FLAG(x) (1 << x)
#define NameToStr(n) #n
#define ValueToStr(v) NameToStr(v)

#define _CONCAT(a,b) a##b
#define CONCAT(a,b) _CONCAT(a,b)

#include "ClassTemplates.h"

#include "Types.h"

#define _KB(x) (x * 1024)
#define _MB(x) (x * 1024 * 1024)
#define _64KB _KB(64)
#define _1MB _MB(1)
#define _2MB _MB(2)
#define _4MB _MB(4)
#define _8MB _MB(8)
#define _16MB _MB(16)
#define _32MB _MB(32)
#define _64MB _MB(64)
#define _128MB _MB(128)
#define _256MB _MB(256)

#include "Utils/Utils.h"

/* 
*	Compile time bitflags relying on __LINE__ to get the right bit index and uses compile time function to check if name is ending with "Flag".
*	A bitflag name must have 'Flag' as an ending. This allows it to create a typedef for the flags type.
*	Example:
*		BEGIN_BITFLAGS_U32(MyFlag)
*			BITFLAG(A)
*			BITFLAG(B)
*			BITFLAG(C)
*		END_BITFLAGS()
* 
*	This creates a struct called MyFlag which holds constant values of type uint32. The values are A = 1, B = 2 and C = 4.
*	It also adds a flag for zero called NONE automatically and a flag called COUNT. NONE is always 0 and COUNT is the number of user defined flags.
*	In this example it is 3.
*	On top of thoes flags, it also adds indices for them. In this case they would be A_INDEX = 1, B_INDEX = 2, C_INDEX = 3
*	And the names as strings: A_STR = "A", B_STR = "B", C_STR = "C"
*	One can create a flags variable by doing:
*		MyFlags tmp = MyFlag::B;
*	Note the added 's' at the end of the type. This is done to clarify that many flags can be in the same variable.
*	Which is the same as writing:
*		uint32 tmp = 2;
*	
*	All functions which makes it work are hidden inside the nested _Internal struct.
* */
#define RS_BEGIN_BITFLAGS(type, field)																					\
	typedef type field##s;																								\
	struct field {																										\
		struct _Internal {																								\
			inline static constexpr const char* fieldName = #field;														\
			inline static constexpr bool containsFlag = RS::Utils::EndsWith(fieldName, "Flag");							\
			static_assert(containsFlag, "Flag name does not contain the last string 'Flag', example field = 'MyFlag'"); \
			typedef type BitFieldType;																					\
			template<BitFieldType N>																					\
			struct FlagStruct {																							\
			static_assert(N < (sizeof(type) * 8), "N is too big");														\
				enum : type { value = 1 << N };																			\
			};																											\
			template<>																									\
			struct FlagStruct<0> {																						\
				enum : type { value = 1 };																				\
			};																											\
			inline static constexpr BitFieldType startLine = __LINE__ + 1; }; /* startLine should be the line right before the first call to BITFLAG, because we want a line diff of 1 for each consecutive FLAG.*/ \
		inline static constexpr _Internal::BitFieldType NONE = 0; // Add a flag for zero, it is always named NONE
#define RS_BEGIN_BITFLAGS_U32(field) RS_BEGIN_BITFLAGS(uint32, field)
#define RS_BEGIN_BITFLAGS_U64(field) RS_BEGIN_BITFLAGS(uint64, field)
#define RS_BITFLAG(name) inline static constexpr _Internal::BitFieldType name = _Internal::FlagStruct<__LINE__ - _Internal::startLine>::value; inline static constexpr _Internal::BitFieldType CONCAT(name, _INDEX) = __LINE__ - _Internal::startLine; inline static constexpr const char* CONCAT(name, _STR) = ValueToStr(name);
// Use this after all calls to BITFLAG!!
#define RS_BITFLAG_COMBO(name, flag) inline static constexpr _Internal::BitFieldType name = flag; inline static constexpr _Internal::BitFieldType CONCAT(name, _INDEX) = __LINE__ - _Internal::startLine; inline static constexpr const char* CONCAT(name, _STR) = ValueToStr(name);
#define RS_END_BITFLAGS() inline static constexpr _Internal::BitFieldType COUNT = __LINE__ - _Internal::startLine; \
	inline static constexpr _Internal::BitFieldType MASK = (1 << COUNT) - 1; };

/*
* Same interface as bitflags but does not restrict to a power of 2.
*/
#define RS_BEGIN_FLAGS(type, field)	\
	typedef type field##s;			\
	struct field {					\
		struct _Internal {			\
			inline static constexpr const char* fieldName = #field;														\
			inline static constexpr bool containsFlag = RS::Utils::EndsWith(fieldName, "Flag");							\
			static_assert(containsFlag, "Flag name does not contain the last string 'Flag', example field = 'MyFlag'"); \
			typedef type FlagType;																						\
			inline static constexpr FlagType startLine = __LINE__; }; /* startLine should be the line right before the first call to FLAGS, because we want a line diff of 1 for each consecutive FLAG.*/ \
		inline static constexpr _Internal::FlagType NONE = 0; // Add a flag for zero, it is always named NONE
#define RS_BEGIN_FLAGS_U32(field) RS_BEGIN_FLAGS(uint32, field)
#define RS_BEGIN_FLAGS_U64(field) RS_BEGIN_FLAGS(uint64, field)
#define RS_FLAG(name) inline static constexpr _Internal::FlagType name = __LINE__ - _Internal::startLine; inline static constexpr const char* CONCAT(name, _STR) = ValueToStr(name);
#define RS_FLAG_V(name, value) inline static constexpr _Internal::FlagType name = value; inline static constexpr const char* CONCAT(name, _STR) = ValueToStr(name);
#define RS_END_FLAGS() inline static constexpr _Internal::FlagType COUNT = __LINE__ - _Internal::startLine; \
	inline static constexpr _Internal::FlagType MASK = (1 << COUNT) - 1; };

// Check if power of 2 and treat 0 as false.
#define IS_POWER_OF_2(x) (x && !(x & (x - 1)))
#define IS_SINGLE_FLAG(flags) IS_POWER_OF_2(flags)
