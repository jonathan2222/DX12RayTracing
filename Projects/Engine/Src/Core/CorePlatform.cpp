#include "PreCompiled.h"
#include "CorePlatform.h"

std::shared_ptr<RS::CorePlatform> RS::CorePlatform::Get()
{
	static std::shared_ptr<CorePlatform> s_Platform = std::make_shared<CorePlatform>();
	return s_Platform;
}

std::string RS::CorePlatform::GetComputerNameStr() const
{
	const uint32 c_MaxStringSize = 256;

	uint32 stringSize = c_MaxStringSize;
	COMPUTER_NAME_FORMAT format = ComputerNameNetBIOS;
	CHAR buffer[c_MaxStringSize];
	bool res = GetComputerNameExA(format, (LPSTR)buffer, (LPDWORD)&stringSize);
	RS_ASSERT(res, "Failed to get computer name!");

	return std::string((char*)buffer);
}

std::string RS::CorePlatform::GetUserNameStr() const
{
	const uint32 c_MaxStringSize = 256;

	uint32 stringSize = c_MaxStringSize;
	CHAR buffer[c_MaxStringSize];
	bool res = GetUserNameA((LPSTR)buffer, (LPDWORD)&stringSize);
	RS_ASSERT(res, "Failed to get user name!");

	return std::string((char*)buffer);
}

std::string RS::CorePlatform::GetConfigurationAsStr() const
{
#if defined(RS_CONFIG_DEBUG)
	return "Debug";
#elif defined(RS_CONFIG_RELEASE)
	return "Release";
#elif defined(RS_CONFIG_PRODUCTION)
	return "Production";
#else
	RS_ASSERT(false, "Unknown Configuration!");
	return "Unknown_Configuration";
#endif
}
