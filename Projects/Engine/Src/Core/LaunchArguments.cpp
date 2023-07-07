#include "PreCompiled.h"
#include "LaunchArguments.h"

std::unordered_map<std::string, RS::LaunchArguments::Param>	RS::LaunchArguments::m_Params;
std::unordered_map<std::string, RS::ParamID> RS::LaunchArguments::m_FixedParamsNameToIDMap;
std::array<bool, RS::LaunchParams::_Internal::c_NumFixedParams>	RS::LaunchArguments::m_FixedParamsUsage;
std::string RS::LaunchArguments::m_ProgramName;
std::string RS::LaunchArguments::m_FullLaunchArgumentsStr;

void RS::LaunchArguments::Init(int argc, char* argv[])
{
    FillFixedParamsList();
    ParseArgs(argc, argv);
    LOG_INFO("Launching {}", m_ProgramName);
    LOG_INFO("Launch arguments: {}", m_FullLaunchArgumentsStr.c_str());
}

void RS::LaunchArguments::Release()
{
    m_Params.clear();
}

bool RS::LaunchArguments::Contains(const std::string& param)
{
    return m_Params.find(param) != m_Params.end();
}

bool RS::LaunchArguments::Contains(ParamID paramID)
{
    if (paramID >= LaunchParams::_Internal::c_NumFixedParams)
        return false;
   
    return m_FixedParamsUsage[paramID];
}

bool RS::LaunchArguments::ContainsAll(std::vector<ParamID> paramIDs)
{
    bool res = true;
    for (auto paramID : paramIDs)
        res &= Contains(paramID);
    return res;
}

bool RS::LaunchArguments::ContainsAny(std::vector<ParamID> paramIDs)
{
    for (auto paramID : paramIDs)
        if (Contains(paramID)) return true;
    return false;
}

void RS::LaunchArguments::ParseArgs(int argc, char* argv[])
{
    m_FullLaunchArgumentsStr = "";
    m_ProgramName = argv[0];

    bool shouldAdd = false;
    Param param;
    for (int i = 1; i < argc; ++i)
    {
        char* arg = argv[i];
        if (arg[0] == '-')
        {
            // Add previous param.
            AddParam(shouldAdd, param);

            param.name = arg+1;
            shouldAdd = true;
        }
        else
        {
            param.args.push_back(arg);
        }

        m_FullLaunchArgumentsStr += i != 1 ? " " + std::string(arg) : arg;
    }

    AddParam(shouldAdd, param);
}

void RS::LaunchArguments::AddParam(bool shouldAdd, const Param& param)
{
    if (shouldAdd)
    {
        std::string paramNameLowercase = Utils::ToLower(param.name);
        auto res = m_Params.insert(std::pair<std::string, Param>(param.name, param));
        if (res.second == false)
        {
            LOG_WARNING("Launch argument '-{}' has already been added.");
        }
        else if (auto it = m_FixedParamsNameToIDMap.find(paramNameLowercase); it != m_FixedParamsNameToIDMap.end())
        {
            m_FixedParamsUsage[it->second] = true;
        }
    }
}

void RS::LaunchArguments::FillFixedParamsList()
{
    using namespace LaunchParams::_Internal;
    for (uint32 i = 0; i < c_NumFixedParams; ++i)
    {
        std::string name = c_ParamNames[i];
        name = Utils::ToLower(name);
        m_FixedParamsNameToIDMap[name] = i;
        m_FixedParamsUsage[i] = false;
    }
}
