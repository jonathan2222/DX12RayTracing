#pragma once

#include <cassert>
#include <vector>
#include <string>

#define LOG_DEBUG_ENABLED 1
#define LOG_ENABLE_WINDOWS_DEBUGGER_LOGGING 1
#define LOG_FLUSH_INTERVAL 2 // In seconds

#define FRAME_BUFFER_COUNT 3

#define RS_CONFIG_DEVELOPMENT defined(RS_CONFIG_DEBUG) || defined(RS_CONFIG_RELEASE)
#include "Utils/Logger.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#ifdef RS_CONFIG_DEVELOPMENT
	#define RS_ASSERT_NO_MSG(exp, ...) {assert(exp);}
	#define RS_ASSERT(exp, ...) {if(!(exp)){LOG_CRITICAL(__VA_ARGS__);} assert(exp); }
#else
	#define RS_ASSERT_NO_MSG(exp, ...) {assert(exp);}
	#define RS_ASSERT(exp, ...) {assert(exp);}
#endif

#define RS_LOG_FILE_PATH "../../Debug/Tmp/"
#define RS_CONFIG_FILE_PATH "../../Assets/Config/EngineConfig.json"
#define RS_SHADER_PATH "../../Assets/Shaders/"
#define RS_TEXTURE_PATH "../../Assets/Textures/"
#define RS_MODEL_PATH "../../Assets/Models/"

#define RS_UNREFERENCED_VARIABLE(v) (void)v
#define FLAG(x) (1 << x)
#define NameToStr(n) #n
#define ValueToStr(v) NameToStr(v)

#include "ClassTemplates.h"

typedef int8_t		int8;
typedef int16_t		int16;
typedef int32_t		int32;
typedef int64_t		int64;

typedef uint8_t		uint8;
typedef uint16_t	uint16;
typedef uint32_t	uint32;
typedef uint64_t	uint64;

#include "Utils/Utils.h"

/* 
*	Compile time bitflags relying on __LINE__ to get the right bit indexand uses compile time function to check if name ending as "Flag".
*	A bitflag name must have 'Flag' as an ending. This allows it to create a typedef for the flags type.
*	Example:
*		BEGIN_BITFLAGS_U32(MyFlag)
*			BITFLAG(A)
*			BITFLAG(B)
*			BITFLAG(C)
*		END_BITFLAGS()
* 
*	This creates a namespace MyFlag which holds constant values of type uint32. The values are A = 1, B = 2 and C = 4.
*	One can create a flags variable by doing:
*		MyFlags tmp = MyFlag::B;
*	Which is the same as writing:
*		uint32 tmp = 2;
* */
#define BEGIN_BITFLAGS(type, field)																						\
    typedef type field##s;																								\
	namespace field {																									\
        constexpr char* fieldName = #field;																				\
		namespace _Internal {																							\
			constexpr bool containsFlag = RS::Utils::ContainsLastStr(fieldName, "Flag");								\
			static_assert(containsFlag, "Flag name does not contain the last string 'Flag', example field = 'MyFlag'"); \
		}																												\
		typedef type _InternalBitFieldType;																				\
		template<_InternalBitFieldType N>																				\
		struct FlagStruct {																								\
			enum : type { value = 1 << N };																				\
		};																												\
		template<>																										\
		struct FlagStruct<0> {																							\
			enum : type { value = 1 };																					\
		};																												\
		constexpr _InternalBitFieldType startLine = __LINE__ + 1;
#define BEGIN_BITFLAGS_U32(field) BEGIN_BITFLAGS(uint32, field)
#define BEGIN_BITFLAGS_U64(field) BEGIN_BITFLAGS(uint64, field)
#define BITFLAG(flag) constexpr _InternalBitFieldType flag = FlagStruct<__LINE__ - startLine>::value;
#define END_BITFLAGS() }