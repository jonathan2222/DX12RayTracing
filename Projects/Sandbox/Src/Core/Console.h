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
			RS_FLAG(ReadOnly)
		RS_END_FLAGS();

		struct FuncArg
		{
			enum class Type : uint32
			{
				Unknown,
				Int,
				Float
			};

			Type type = Type::Unknown;
			std::string name = "";
			union
			{
				union
				{
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
			FuncArg(Type type) : type(type), name(""), value(0) {}
			FuncArg(const std::string& name) : type(Type::Unknown), name(name), value(0) {}
			FuncArg(Type type, const std::string& name) : type(type), name(name), value(0) {}
			FuncArg(Type type, const std::string& name, uint64 value) : type(type), name(name), value(value) {}
		};

		struct FuncArgsInternal : public std::vector<FuncArg>
		{
			FuncArgsInternal() : vector() {};
			FuncArgsInternal(std::initializer_list<FuncArg> _Ilist) : vector(_Ilist) {}

			std::optional<FuncArg> Get(const std::string& name) const
			{
				auto it = std::find_if(begin(), end(), [name](const FuncArg& arg)->bool { return arg.name == name; });
				if (it == end())
					return {};
				return *it;
			}

			std::optional<FuncArg> GetUnnamed(uint32 index) const
			{
				uint32 currentIndex = 0;
				auto it = std::find_if(begin(), end(), [&](const FuncArg& arg)->bool
					{
						if (arg.name.empty() && arg.type != FuncArg::Type::Unknown)
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
		using Func = std::function<void(FuncArgs)>;
	public:
		RS_NO_COPY_AND_MOVE(Console);
		Console() = default;
		~Console() = default;

		static Console* Get();

		void Init();
		void Release();

		template<typename T>
		bool AddVar(const std::string& name, T& var, Flags flags = Flag::NONE);

		/*
		* A function can only hold arguments of type unknown, int32, and float.
		*/
		bool AddFunction(const std::string& name, Func func);
		bool ValidateFuncArgs(FuncArgs args, FuncArgs validArgs, bool strict = false);

		bool RemoveVar(const std::string& name);

		void Render();
		void RenderStats();

		template<typename... Args>
		void Print(const char* format, Args&&...args);
		template<typename... Args>
		void Print(const ImVec4& color, const char* format, Args&&...args);

		void Print(const char* txt);
		void Print(const ImVec4& color, const char* txt);

		// TODO: thread safe these?
		void Enable() { m_Enabled = true; }
		void Disable() { m_Enabled = false; }
		[[nodiscard]] bool IsEnabled() const { return m_Enabled; }

	private:
		struct Line
		{
			std::string str;
			ImVec4		color = { 1.0f, 1.0f, 1.0f, 1.0f };
		};

		enum class Type : uint32
		{
			Unknown = 0,
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

		enum class TypeInfo : uint32
		{
			NumInts = (uint32)Type::Int64 - (uint32)Type::Int8 + 1,
			NumUInts = (uint32)Type::UInt64 - (uint32)Type::UInt8 + 1,
			NumFloats = (uint32)Type::Float - (uint32)Type::Double + 1,
		};
		inline static bool IsTypeInt(Type type) { return type >= Type::Int8 && type <= Type::Int64; }
		inline static bool IsTypeUInt(Type type) { return type >= Type::UInt8 && type <= Type::UInt64; }
		inline static bool IsTypeFloat(Type type) { return type >= Type::Float && type <= Type::Double; }
		inline static bool IsOfSameType(Type a, Type b)
		{
			return a == b || (IsTypeInt(a) && IsTypeInt(b)) || (IsTypeUInt(a) && IsTypeUInt(b)) || (IsTypeFloat(a) && IsTypeFloat(b));
		}
		inline static bool IsTypeVariable(Type type) { return IsTypeInt(type) || IsTypeUInt(type) || IsTypeFloat(type); }
		static int GetByteCountFromType(Type type);

		struct Variable
		{
			std::string	name;
			Type		type = Type::Unknown;
			Flags		flags = Flag::NONE;
			void*		pVar = nullptr;
			Func		func;
			Variable() = default;
			Variable(const std::string& name, Type type, Flags flags, void* pVar, Func func)
				: name(name), type(type), flags(flags), pVar(pVar), func(func)
			{}
		};
		bool AddVarInternal(const std::string& name, const Variable& var);

		inline static int InputTextCallback(ImGuiInputTextCallbackData* data)
		{
			Console*pConsole = (Console*)data->UserData;
			return pConsole->HandleInputText(data);
		}

		int HandleInputText(ImGuiInputTextCallbackData* data);
		void ExecuteCommand(const std::string& cmd);

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

		static std::string VarTypeToString(Type type);
		static std::string VarValueToString(Type type, void* value);
		static Type StringToVarValue(Type type, const std::string& str, void* refValue, int _internal = 0);
		static Type StringToVarType(const std::string& str);

	private:
		std::mutex m_VariablesMutex;
		std::unordered_map<std::string, Variable> m_VariablesMap;
		std::vector<Line> m_History; // All history of prints to the console.
		std::vector<std::string> m_CommandHistory; // All history of command executions to the console.
		std::string m_CurrentSearchableItem;
		std::vector<std::string> m_SearchableItems; // All searchable commands are here.
		std::vector<MatchedSearchItem> m_MatchedSearchList; // Items that were matched with when searched.

		bool	m_Enabled = false;
		char	m_InputBuf[256];

		int32	m_CurrentCmdHistoryIndexOffset = -1;
		float	m_DisplayHeightRatio = 0.25f;
		float	m_Opacity = 0.75f;
		uint32	m_SearchPopupMaxDisplayCount = 5;
		uint32	m_CurrentMatchedVarIndex = UINT32_MAX;
	};

	template<typename T>
	inline bool Console::AddVar(const std::string& name, T& var, Console::Flags flags)
	{
		return AddVarInternal(name, Variable(name, Type::Unknown, flags, &var, nullptr));
	}

	template<typename... Args>
	inline void Console::Print(const char* format, Args&&...args)
	{
		std::string str = RS::Utils::Format(format, std::forward<Args>(args)...);
		Print(str.c_str());
	}

	template<typename... Args>
	inline void Console::Print(const ImVec4& color, const char* format, Args&&...args)
	{
		std::string str = RS::Utils::Format(format, std::forward<Args>(args)...);
		Print(color, str.c_str());
	}

#define RS_ADD_VAR(type, enumType) \
	template<>	\
	inline bool Console::AddVar(const std::string& name, type& var, Console::Flags flags) \
	{ \
		return AddVarInternal(name, Variable( name, enumType, flags, &var, nullptr )); \
	};

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