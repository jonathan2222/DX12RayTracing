#pragma once

#pragma warning( push )
#pragma warning( disable : 26444 )
#pragma warning( disable : 26451 )
#pragma warning( disable : 28020 )
#pragma warning( disable : 4100)
#include <nlohmann/json.hpp>

#include "Defines.h"

using json = nlohmann::json;

namespace RS
{
	class Config
	{
	public:
		Config() = default;

		static Config* Get();

		void Init(const std::string& fileName);

		template<typename T>
		T Fetch(const std::string& name, T defaultValue);

	private:
		enum class ConfigType { STRING = 0, INT, FLOAT, BOOL };
		const std::string ConfigTypeStr[4] = { "STRING", "INT", "FLOAT", "BOOL" };
		struct ConfigData
		{
			ConfigType type;
			union
			{
				char str[256];
				int32 i;
				float f;
				bool b;
			};
		};

		void ReadFile(const std::string& fileName);
		void Load(const std::string& str, json& j);

		template<typename IteratorType, typename T>
		bool Validate(IteratorType& it, const std::string& name, ConfigType type, T defaultValue);

	private:
		std::unordered_map<std::string, ConfigData> m_map;

	};

	template<typename T>
	inline T Config::Fetch(const std::string& name, T defaultValue)
	{
		RS_ASSERT(false, "Cannot fetch \"{0}\" from config map, type \"{1}\" is not supported!", name.c_str(), typeid(T).name());
		return (T)0;
	}

	template<typename IteratorType, typename T>
	inline bool Config::Validate(IteratorType& it, const std::string& name, ConfigType type, T defaultValue)
	{
		bool succeeded = true;
		if (it == m_map.end())
		{
			succeeded = false;
		}
		else if (it->second.type != type)
		{
			LOG_WARNING("Config entry is not of type {}!", ConfigTypeStr[(size_t)type]);
			succeeded = false;
		}

		if (!succeeded)
		{
			LOG_WARNING("Did not find \"{0}\" in config map! Using default value: {1}", name.c_str(), defaultValue);
		}

		return succeeded;
	}

	template<>
	inline int32 Config::Fetch(const std::string& name, int32 defaultValue)
	{
		std::unordered_map<std::string, ConfigData>::iterator it = m_map.find(name);
		if (Validate(it, name, ConfigType::INT, defaultValue))
			return it->second.i;
		else
			return defaultValue;
	}

	template<>
	inline uint32 Config::Fetch(const std::string& name, uint32 defaultValue)
	{
		return (uint32)Fetch<int32>(name, (int32)defaultValue);
	}

	template<>
	inline float Config::Fetch(const std::string& name, float defaultValue)
	{
		std::unordered_map<std::string, ConfigData>::iterator it = m_map.find(name);
		if (Validate(it, name, ConfigType::FLOAT, defaultValue))
			return it->second.f;
		else
			return defaultValue;
	}

	template<>
	inline std::string Config::Fetch(const std::string& name, std::string defaultValue)
	{
		std::unordered_map<std::string, ConfigData>::iterator it = m_map.find(name);
		if (Validate(it, name, ConfigType::STRING, defaultValue))
			return std::string(it->second.str);
		else
			return defaultValue;
	}

	template<>
	inline bool Config::Fetch(const std::string& name, bool defaultValue)
	{
		std::unordered_map<std::string, ConfigData>::iterator it = m_map.find(name);
		if (Validate(it, name, ConfigType::BOOL, defaultValue))
			return it->second.b;
		else
			return defaultValue;
	}
}

#pragma warning( pop )
