#pragma once
/*
#pragma warning( push )
#pragma warning( disable : 26444 )
#pragma warning( disable : 26451 )
#pragma warning( disable : 28020 )
#pragma warning( disable : 4100)
#include <nlohmann/json.hpp>
*/
#include "Defines.h"

//using json = nlohmann::json;
#include "Core/VMap.h"

namespace RS
{
	class Config
	{
	public:
		Config() = default;

		static Config* Get();

		void Init(const std::string& fileName);
		void Destroy();

		template<typename T>
		T Fetch(const std::string& name, T defaultValue);

		inline static VMap Map = {};

		void Load();
		void Save();

	private:
		std::string m_FilePath;
	};

	template<typename T>
	inline T Config::Fetch(const std::string& name, T defaultValue)
	{
		RS_ASSERT(false, "Cannot fetch \"{0}\" from config map, type \"{1}\" is not supported!", name.c_str(), typeid(T).name());
		return (T)0;
	}

	template<>
	inline int32 Config::Fetch(const std::string& name, int32 defaultValue)
	{
		VMap* data = Map.GetIfExists(name);
		if (data == nullptr)
		{
			LOG_WARNING("Did not find \"{0}\" in config map! Using default value: {1}", name.c_str(), defaultValue);
			return defaultValue;
		}
		if (!data->IsOfType<int32>())
		{
			LOG_WARNING("Config entry is not of type int32! Using default value: {}", defaultValue);
			return defaultValue;
		}
		return *data;
	}

	template<>
	inline uint32 Config::Fetch(const std::string& name, uint32 defaultValue)
	{
		VMap* data = Map.GetIfExists(name);
		if (data == nullptr)
		{
			LOG_WARNING("Did not find \"{0}\" in config map! Using default value: {1}", name.c_str(), defaultValue);
			return defaultValue;
		}
		if (!data->IsOfType<uint32>())
		{
			LOG_WARNING("Config entry is not of type uint32! Using default value: {}", defaultValue);
			return defaultValue;
		}
		return *data;
	}

	template<>
	inline float Config::Fetch(const std::string& name, float defaultValue)
	{
		VMap* data = Map.GetIfExists(name);
		if (data == nullptr)
		{
			LOG_WARNING("Did not find \"{0}\" in config map! Using default value: {1}", name.c_str(), defaultValue);
			return defaultValue;
		}
		if (!data->IsOfType<float>())
		{
			LOG_WARNING("Config entry is not of type float! Using default value: {}", defaultValue);
			return defaultValue;
		}
		return *data;
	}

	template<>
	inline std::string Config::Fetch(const std::string& name, std::string defaultValue)
	{
		VMap* data = Map.GetIfExists(name);
		if (data == nullptr)
		{
			LOG_WARNING("Did not find \"{0}\" in config map! Using default value: {1}", name.c_str(), defaultValue);
			return defaultValue;
		}
		if (!data->IsOfType<std::string>())
		{
			LOG_WARNING("Config entry is not of type string! Using default value: {}", defaultValue);
			return defaultValue;
		}
		return *data;
	}

	template<>
	inline bool Config::Fetch(const std::string& name, bool defaultValue)
	{
		VMap* data = Map.GetIfExists(name);
		if (data == nullptr)
		{
			LOG_WARNING("Did not find \"{0}\" in config map! Using default value: {1}", name.c_str(), defaultValue);
			return defaultValue;
		}
		if (!data->IsOfType<bool>())
		{
			LOG_WARNING("Config entry is not of type bool! Using default value: {}", defaultValue);
			return defaultValue;
		}
		return *data;
	}
}

//#pragma warning( pop )
