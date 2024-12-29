#pragma once

#include <mutex>
#include <imgui.h>
#include <optional>

namespace RS
{
	class Console
	{
	public:
		RS_BEGIN_FLAGS_U32(Flag)
			RS_FLAG(ReadOnly) // Does not work on Functions
		RS_END_FLAGS();

		struct FuncArg
		{
			RS_BEGIN_BITFLAGS_U32(TypeFlag)
				RS_BITFLAG(Bool)
				RS_BITFLAG(Int)
				RS_BITFLAG(Float)
				RS_BITFLAG(Named)
				RS_BITFLAG(Function)
			RS_END_BITFLAGS();

			TypeFlags type = TypeFlag::NONE;
			std::string name = "";
			union
			{
				union
				{
					bool b;
					int32 i32;
					uint32 ui32;
					int64 i64;
					uint64 ui64;
					float f;
					double d;
				} types;
				uint64 value = 0;
			};
			FuncArg() = default;
			FuncArg(TypeFlags type) : type(type), name(""), value(0) {}
			FuncArg(const std::string& name) : type(TypeFlag::Named), name(name), value(0) {}
			FuncArg(TypeFlags type, const std::string& name) : type(type), name(name), value(0) {}
			FuncArg(TypeFlags type, const std::string& name, uint64 value) : type(type), name(name), value(value) {}
		};

		struct FuncArgsInternal : public std::vector<FuncArg>
		{
			FuncArgsInternal() : vector() {};
			FuncArgsInternal(std::initializer_list<FuncArg> _Ilist) : vector(_Ilist) {}

			std::optional<FuncArg> Get(uint32 index) const
			{
				if (index >= size())
					return {};
				return at(index);
			}

			std::optional<FuncArg> Get(const std::string& name) const
			{
				if (empty())
					return {};
				auto it = std::find_if(begin(), end(), [name](const FuncArg& arg)->bool { return arg.name == name; });
				if (it == end())
					return {};
				return *it;
			}

			std::optional<FuncArg> GetUnnamed(uint32 index) const
			{
				if (empty())
					return {};
				uint32 currentIndex = 0;
				auto it = std::find_if(begin(), end(), [&](const FuncArg& arg)->bool
					{
						if (arg.name.empty() && (arg.type != FuncArg::TypeFlag::NONE && (arg.type & FuncArg::TypeFlag::Named) != 0))
						{
							currentIndex++;
							if (currentIndex == index)
								return true;
						}
						return false;
					}
				);
				if (it == end())
					return {};
				return *it;
			}
		};

		using FuncArgs = const FuncArgsInternal&;
		using Func = std::function<bool(FuncArgs)>;

		enum class Type : uint32
		{
			Unknown = 0,
			Bool,
			Int8,
			Int16,
			Int32,
			Int64,
			UInt8,
			UInt16,
			UInt32,
			UInt64,
			Float,
			Double,
			Function
		};
		inline static std::vector<Type> s_VariableTypes = { Type::Bool, Type::Int8, Type::Int16, Type::Int32, Type::Int64, Type::UInt8, Type::UInt16, Type::UInt32, Type::UInt64, Type::Float, Type::Double };

		struct Variable
		{
			std::string	name	= "";
			Type		type	= Type::Unknown;
			Flags		flags	= Flag::NONE;
			void*		pVar	= nullptr;
			Func		func	= nullptr;
			Variable() = default;
			Variable(const std::string& name, Type type, Flags flags, void* pVar, Func func, const std::string& docs)
				: name(name), type(type), flags(flags), pVar(pVar), func(func), documentation(docs)
			{}

			std::vector<FuncArg::TypeFlags> searchableTypes;
			std::string documentation;
		};
	public:
		RS_NO_COPY_AND_MOVE(Console);
		~Console() = default;

		static Console* Get();

		void Init();
		void Release();

		template<typename T>
		bool AddVar(const std::string& name, T& var, Flags flags = Flag::NONE, const std::string& docs = "");
		
		template<typename T>
		bool AddVar(const std::string& name, T& var, const std::vector<FuncArg::TypeFlags>& searchableTypes, Flags flags = Flag::NONE, const std::string& docs = "");

		RS_BEGIN_BITFLAGS_U32(ValidateFuncArgsFlag)
			RS_BITFLAG(ArgCountMustMatch)
			RS_BITFLAG(TypeMatchOnly)
			RS_BITFLAG_COMBO(Strict, ArgCountMustMatch)
		RS_END_BITFLAGS();

		uint64 GetStateHash() const;

		/*
		* A function can only hold arguments of type unknown, int32, and float.
		*/
		bool AddFunction(const std::string& name, Func func, Flags flags = Flag::NONE, const std::string& docs = "");
		/*
		* searchableTypes specifies which variables will show up as potensial matches after this function.
		*/
		bool AddFunction(const std::string& name, Func func, const std::vector<FuncArg::TypeFlags>& searchableTypes, Flags flags = Flag::NONE, const std::string& docs = "");
		static bool ValidateFuncArgs(FuncArgs args, FuncArgs validArgs, ValidateFuncArgsFlags flags = ValidateFuncArgsFlag::NONE);

		bool RemoveVar(const std::string& name);

		Variable* GetVariable(const std::string& name);
		std::vector<Variable> GetVariables() const;
		std::vector<Variable> GetVariables(const std::vector<Type>& includeList) const;

		void ExecuteCommand(const std::string& cmd);

		void Render();

		template<typename... Args>
		void Print(const std::string_view, Args&&...args);
		template<typename... Args>
		void Print(const ImVec4& color, const std::string_view, Args&&...args);

		void Print(const char* txt);
		void Print(const ImVec4& color, const char* txt);

		// TODO: thread safe these?
		void Enable() { m_Enabled = true; }
		void Disable() { m_Enabled = false; }
		[[nodiscard]] bool IsEnabled() const { return m_Enabled; }

		static std::string VarTypeToString(Type type);
		static std::string VarValueToString(Type type, void* value);
		static Type StringToVarValue(Type type, const std::string& str, void* refValue, int _internal = 0);
		static Type StringToVarType(const std::string& str);

		enum class TypeInfo : uint32
		{
			NumInts = (uint32)Type::Int64 - (uint32)Type::Int8 + 1,
			NumUInts = (uint32)Type::UInt64 - (uint32)Type::UInt8 + 1,
			NumFloats = (uint32)Type::Float - (uint32)Type::Double + 1,
		};
		inline static bool IsTypeBool(Type type) { return type == Type::Bool; }
		inline static bool IsTypeInt(Type type) { return type >= Type::Int8 && type <= Type::Int64; }
		inline static bool IsTypeUInt(Type type) { return type >= Type::UInt8 && type <= Type::UInt64; }
		inline static bool IsTypeFloat(Type type) { return type >= Type::Float && type <= Type::Double; }
		inline static bool IsTypeFunction(Type type) { return type == Type::Function; }
		inline static bool IsOfSameType(Type a, Type b)
		{
			return a == b || (IsTypeBool(a) && IsTypeBool(b)) || (IsTypeInt(a) && IsTypeInt(b)) || (IsTypeUInt(a) && IsTypeUInt(b)) || (IsTypeFloat(a) && IsTypeFloat(b));
		}
		inline static bool IsTypeVariable(Type type) { return IsTypeBool(type) || IsTypeInt(type) || IsTypeUInt(type) || IsTypeFloat(type); }
		static int GetByteCountFromType(Type type);
		static FuncArg::TypeFlags TypeToFuncArgType(Type type)
		{
			FuncArg::TypeFlags types = FuncArg::TypeFlag::NONE;
			if (IsTypeBool(type)) types |= FuncArg::TypeFlag::Bool;
			if (IsTypeInt(type) || IsTypeUInt(type)) types |= FuncArg::TypeFlag::Int;
			if (IsTypeFloat(type)) types |= FuncArg::TypeFlag::Float;
			if (type == Type::Function) types |= FuncArg::TypeFlag::Function;
			return types;
		}
	private:
		Console() = default;

		struct Line
		{
			std::string str;
			ImVec4		color = { 1.0f, 1.0f, 1.0f, 1.0f };
		};

		bool AddVarInternal(const std::string& name, const Variable& var);

		inline static int InputTextCallback(ImGuiInputTextCallbackData* data)
		{
			Console*pConsole = (Console*)data->UserData;
			return pConsole->HandleInputText(data);
		}

		int HandleInputText(ImGuiInputTextCallbackData* data);

		void Search(const std::string& searchable);
		struct MatchedSearchItem
		{
			int32 matchedCount = -1;
			int32 startIndex = -1;
			std::string fullLine;
			std::vector<Line> parts;
		};
		void Search(const std::string& searchableLine, const std::vector<std::string>& searchStrings, std::vector<MatchedSearchItem>& refIntermediateMatchedList);
		bool SearchSinglePart(const std::string& searchWord, const std::string& searchableLine, int& outMatchedCount, int& outStartIndex);

		std::string PerformSearchCompletion();
		void ComputeCurrentSearchableItem(const char* searchableLine);
		void SetValidSearchableArgTypesFromCMD(const std::string& cmd);

		void UpdateStateHash(const Variable& var);

	private:
		std::mutex m_VariablesMutex;
		std::unordered_map<std::string, Variable> m_VariablesMap;

		std::vector<Line> m_History; // All history of prints to the console.
		std::vector<std::string> m_CommandHistory; // All history of command executions to the console.

		uint32		m_CurrentSearchableItemStartPos = 0;
		std::string m_CurrentSearchableItem;
		std::vector<std::string> m_SearchableItems; // All searchable commands are here.
		std::vector<MatchedSearchItem> m_MatchedSearchList; // Items that were matched with when searched.
		std::vector<FuncArg::TypeFlags> m_ValidSearchableArgTypes;

		bool	m_Enabled = false;
		char	m_InputBuf[512];

		int32	m_CurrentCmdHistoryIndexOffset = -1;
		float	m_DisplayHeightRatio = 0.25f;
		float	m_Opacity = 0.75f;
		uint32	m_SearchPopupMaxDisplayCount = 5;
		uint32	m_CurrentMatchedVarIndex = UINT32_MAX;

		bool	m_DisplaySearchResultsForEmptySearchWord = false;

		uint64	m_StateHash = 0xdeadbeef;
	};

	template<typename T>
	inline bool Console::AddVar(const std::string& name, T& var, Console::Flags flags, const std::string& docs)
	{
		return AddVarInternal(name, Variable(name, Type::Unknown, flags, &var, nullptr, docs));
	}

	template<typename T>
	inline bool Console::AddVar(const std::string& name, T& var, const std::vector<FuncArg::TypeFlags>& searchableTypes, Flags flags, const std::string& docs)
	{
		Variable varObj(name, Type::Unknown, flags, &var, nullptr, docs);
		varObj.searchableTypes = searchableTypes;
		return AddVarInternal(name, varObj);
	}

	template<typename... Args>
	inline void Console::Print(const std::string_view format, Args&&...args)
	{
		const std::string str = std::vformat(format, std::make_format_args(std::forward<Args>(args)...));
		Print(str.c_str());
	}

	template<typename... Args>
	inline void Console::Print(const ImVec4& color, const std::string_view format, Args&&...args)
	{
		const std::string str = std::vformat(format, std::make_format_args(std::forward<Args&>(args)...));
		Print(color, str.c_str());
	}

#define RS_ADD_VAR(type, enumType) \
	template<>	\
	inline bool Console::AddVar(const std::string& name, type& var, Console::Flags flags, const std::string& docs) \
	{ \
		return AddVarInternal(name, Variable( name, enumType, flags, &var, nullptr, docs)); \
	}; \
	template<>	\
	inline bool Console::AddVar(const std::string& name, type& var, const std::vector<FuncArg::TypeFlags>& searchableTypes, Console::Flags flags, const std::string& docs) \
	{ \
		Variable varObj(name, enumType, flags, &var, nullptr, docs); \
		varObj.searchableTypes = searchableTypes; \
		return AddVarInternal(name, varObj); \
	};

	RS_ADD_VAR(bool, Console::Type::Bool);
	RS_ADD_VAR(int8, Console::Type::Int8);
	RS_ADD_VAR(int16, Console::Type::Int16);
	RS_ADD_VAR(int32, Console::Type::Int32);
	RS_ADD_VAR(int64, Console::Type::Int64);
	RS_ADD_VAR(uint8, Console::Type::UInt8);
	RS_ADD_VAR(uint16, Console::Type::UInt16);
	RS_ADD_VAR(uint32, Console::Type::UInt32);
	RS_ADD_VAR(uint64, Console::Type::UInt64);
	RS_ADD_VAR(float, Console::Type::Float);
	RS_ADD_VAR(double, Console::Type::Double);

#undef RS_ADD_VAR(type, enumType) 

}

// Example usage: RS_ADD_GLOBAL_CONSOLE_VAR(bool, "Canvas.debug1", g_Debug1, false, "A debug var");
#define RS_ADD_GLOBAL_CONSOLE_VAR(type, name, var, defaultValue, docs) static inline type var = defaultValue; \
	class _CONCAT(_RSGlobalConsoleVar,var) \
	{ \
	public: \
		_CONCAT(_RSGlobalConsoleVar, var)() \
		{ \
			RS::Console::Get()->AddVar<type>(name, var, RS::Console::Flag::NONE, docs); \
		}\
	}; \
	static inline _CONCAT(_RSGlobalConsoleVar, var) _CONCAT(g_RSGlobalConsoleVar, var) = _CONCAT(_RSGlobalConsoleVar, var)();

// Example usage: RS_ADD_GLOBAL_READONLY_CONSOLE_VAR(bool, "Canvas.debug1", g_Debug1, false, "A debug var");
#define RS_ADD_GLOBAL_READONLY_CONSOLE_VAR(type, name, var, defaultValue, docs) static inline type var = defaultValue; \
	class _CONCAT(_RSGlobalRConsoleVar,var) \
	{ \
	public: \
		_CONCAT(_RSGlobalRConsoleVar, var)() \
		{ \
			RS::Console::Get()->AddVar<type>(name, var, RS::Console::Flag::ReadOnly, docs); \
		}\
	}; \
	static inline _CONCAT(_RSGlobalRConsoleVar, var) _CONCAT(g_RSGlobalRConsoleVar, var) = _CONCAT(_RSGlobalRConsoleVar, var)();