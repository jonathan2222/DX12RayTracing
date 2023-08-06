#include "PreCompiled.h"
#include "CorePlatform.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include <stb_image.h>

#include "GUI/LogNotifier.h"

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

std::unique_ptr<RS::CorePlatform::Image> RS::CorePlatform::LoadImageData(const std::string& path, Format requestedFormat, ImageFlags flags)
{
	std::string texturePath = RS_TEXTURE_PATH + path;
	FormatInfo requestedFormatInfo = GetFormatInfo(requestedFormat);

	std::unique_ptr<RS::CorePlatform::Image> pImage = std::make_unique<RS::CorePlatform::Image>();
	int width, height, channelCount;
	const int requestedChannelCount = requestedFormatInfo.channelCount;
	pImage->pData = stbi_load(texturePath.c_str(), &width, &height, &channelCount, requestedChannelCount);
	if (pImage->pData == nullptr)
	{
		LOG_ERROR("Could not load texture!Reason: {}", stbi_failure_reason());
		RS_NOTIFY_ERROR("Could not load texture!Reason: {}", stbi_failure_reason());
		return nullptr;
	}

	if (flags & ImageFlag::FLIP_Y)
	{
		size_t size = width * requestedChannelCount;
		for (int y = 0; y < height / 2; y++)
		{
			int top = y * width * requestedChannelCount;
			int bot = (height - y - 1) * width * requestedChannelCount;
			std::swap_ranges(pImage->pData + top, pImage->pData + top + size, pImage->pData + bot);
		}
	}

	pImage->format = GetFormat((uint32)requestedChannelCount, 8, requestedFormatInfo.formatType);
	pImage->width = (uint32)width;
	pImage->height = (uint32)height;

	return std::move(pImage);
}

RS::CorePlatform::Image::~Image()
{
	if (pData)
		stbi_image_free(pData);
	pData = nullptr;
	width = 0;
	height = 0;
	format = RS_FORMAT_UNKNOWN;
}
