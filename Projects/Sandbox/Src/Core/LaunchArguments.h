#pragma once

namespace RS
{
	typedef uint32 ParamID;
	namespace LaunchParams
	{
		// ParamID variables for all launchParams.
#define DEF_LAUNCH_PARAM(name, numArgs, desc) constexpr ParamID name = __LINE__ - 1; // Can do this as long as LaunchParams.h start using these macros at the first line.
#include "LaunchParams.h"
#undef DEF_LAUNCH_PARAM

		namespace _Internal
		{
			struct LaunchParamInfo
			{
				ParamID	value; // Unique ID for this param.
				uint32	numArgs;
				char* name;
				char* description;
			};

			// List of param names in the same order as the ParamIDs.
			constexpr char* c_ParamNames[] = {
#define DEF_LAUNCH_PARAM(name, numArgs, desc) #name,
#include "LaunchParams.h"
#undef DEF_LAUNCH_PARAM
			};

			// List of full param infos in the same order as the ParamIDs.
#define DEF_LAUNCH_PARAM(name, numArgs, desc) {(__LINE__ - 1), numArgs, #name, desc},
			constexpr LaunchParamInfo c_Params[] = {
#include "LaunchParams.h"
#undef DEF_LAUNCH_PARAM
			};
			constexpr uint32 c_NumFixedParams = ARRAYSIZE(c_ParamNames);
		}
	}

	class LaunchArguments
	{
	public:
		RS_NO_COPY_AND_MOVE(LaunchArguments);
		LaunchArguments() = default;
		~LaunchArguments() = default;

		static void Init(int argc, char* argv[]);
		static void Release();

		/*
		* Search through all params that starts with a '-' sign.
		*/
		static bool Contains(const std::string& param);

		/*
		* Fast lookup into the fixed launch params.
		*/
		static bool Contains(ParamID paramID);
		static bool ContainsAll(std::vector<ParamID> paramIDs);
		static bool ContainsAny(std::vector<ParamID> paramIDs);

	private:
		struct Param
		{
			std::string name;
			std::vector<std::string> args;
		};

		static void ParseArgs(int argc, char* argv[]);
		static void AddParam(bool shouldAdd, const Param& param);
		static void FillFixedParamsList();
	private:
		static std::unordered_map<std::string, Param>						m_Params;
		static std::unordered_map<std::string, ParamID>						m_FixedParamsNameToIDMap; // All names are lowercase.
		static std::array<bool, LaunchParams::_Internal::c_NumFixedParams>	m_FixedParamsUsage;
		static std::string													m_ProgramName;
		static std::string													m_FullLaunchArgumentsStr;
	};
}