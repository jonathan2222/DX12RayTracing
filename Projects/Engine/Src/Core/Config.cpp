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
	m_FilePath = fileName;
	Load();
}

void Config::Destroy()
{
	Save();
}

void Config::Load()
{
	std::optional<std::string> errorMessage;
	Map = VMap::ReadFromDisk(m_FilePath, errorMessage);
	if (errorMessage.has_value())
		LOG_ERROR("Failed to load Config file! Using defaults. Msg: {}", errorMessage->c_str());
}

void Config::Save()
{
	std::optional<std::string> errorMessage;
	VMap::WriteToDisk(Map, m_FilePath, errorMessage);
	if (errorMessage.has_value())
		LOG_ERROR("Failed to save Config file! Msg: {}", errorMessage->c_str());
}
