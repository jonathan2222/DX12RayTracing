#include "PreCompiled.h"
#include "Config.h"

#include <fstream>

#include <iostream>

using namespace RS;

Config* Config::Get()
{
	static Config config;
	return &config;
}

void Config::Init(const std::string& fileName)
{
	ReadFile(fileName);
}

void Config::ReadFile(const std::string& fileName)
{
	std::ifstream file(fileName);

	json j;

	if (file.is_open())
	{
		file >> j;
		file.close();
		LOG_SUCCESS("Loaded engine config!");

		Load("", j);
	}
	else
	{
		LOG_WARNING("Failed to load engine config!");
	}
}

#pragma warning( push )
#pragma warning( disable : 26444 )
void Config::Load(const std::string& str, json& j)
{
	for (auto it = j.begin(); it != j.end(); it++) {
		std::string newKey = str + (str.empty() ? "" : "/") + it.key();
		if (it->is_object())
			Load(newKey, *it);
		else
		{
			ConfigData data = {};
			if (it->is_boolean())
			{
				data.b = (bool)it.value();
				data.type = ConfigType::BOOL;
			}
			else if (it->is_number_integer())
			{
				data.i = (int)it.value();
				data.type = ConfigType::INT;
			}
			else if (it->is_number_float())
			{
				data.f = (float)it.value();
				data.type = ConfigType::FLOAT;
			}
			else if (it->is_string())
			{
				std::string strValue = (std::string)it.value();
				memcpy(data.str, strValue.c_str(), 256);
				data.type = ConfigType::STRING;
			}
			m_map[newKey] = data;
		}
	}
}
#pragma warning( pop )
