#include "PreCompiled.h"
#include "CorePlatform.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include <stb_image.h>

#include "GUI/LogNotifier.h"

#include <thread>

RS::CorePlatform::CorePlatform()
{
	m_sTemporaryDirectoryPath = CreateTemporaryPath();
}

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

std::unique_ptr<RS::CorePlatform::Image> RS::CorePlatform::LoadImageData(const std::string& path, Format requestedFormat, ImageFlags flags, bool isInternalPath)
{
	std::string texturePath = Engine::GetDataFilePath(isInternalPath) + RS_TEXTURE_PATH + path;
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

RS::CorePlatform::BinaryFile RS::CorePlatform::LoadBinaryFile(const std::string& path, uint32 offset)
{
	BinaryFile file;

	FILE* pFile = stbi__fopen(path.c_str(), "rb");
	uint8* pData = nullptr;
	if (!pFile)
	{
		LOG_ERROR("[LoadFileData] Cannot open file!");
		return file;
	}

	fseek(pFile, 0, SEEK_END);
	file.size = ftell(pFile);
	fseek(pFile, (long)offset, SEEK_SET);

	uint8* pContent = new uint8[file.size];
	fread(pContent, 1, file.size, pFile);

	fclose(pFile);

	file.pData = std::unique_ptr<uint8>(pContent);
	return file;
}

std::string RS::CorePlatform::GetLastErrorString()
{
	DWORD errorCode = GetLastError();
	
	LPTSTR errorText = NULL;
	FormatMessage(
	    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
	    NULL,
	    errorCode,
	    MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
	    (LPTSTR)&errorText,  // output 
	    0, // minimum size for output buffer
	    NULL);
	if (errorText != NULL)
	{
	    std::string str = RS::Utils::ToString(std::wstring(errorText));
	    LOG_ERROR("Failed to start renderdoccmd process! {}", errorCode, str.c_str());
	    LocalFree(errorText);
	    errorText = NULL;
		return str;
	}
	return "?";
}

void RS::CorePlatform::SetCurrentThreadName(const std::string& name)
{
	std::wstring wname = Utils::ToWString(name);
	HRESULT hr = SetThreadDescription(GetCurrentThread(), wname.c_str());
	RS_ASSERT(SUCCEEDED(hr), "Failed to set name of current thread!");
}

void RS::CorePlatform::ThreadSleep(uint64 milliseconds)
{
	auto delay = std::chrono::duration<uint, std::milli>(milliseconds);
	std::this_thread::sleep_for(delay);
}

uint RS::CorePlatform::GetCoreCount(uint defaultCount)
{
	uint hardwareCount = (uint)std::thread::hardware_concurrency();
	return hardwareCount == 0u ? defaultCount : hardwareCount;
}

#include <shlobj.h> // Used for SHGetFolderPathW
std::string RS::CorePlatform::CreateTemporaryPath()
{
	WCHAR documentsPath[MAX_PATH];
	HRESULT result = SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, documentsPath);
	if (result != S_OK)
	{
		std::string resultPath = "TemporaryData/";
		resultPath = Engine::GetTempFilePath() + resultPath;
		LOG_WARNING("Could not find OS Documents folder! Defaulting to {}", resultPath.c_str());
		return resultPath;
	}

	std::string resultPath = Utils::ToString(documentsPath);
	if (resultPath[resultPath.size() - 1] != '/')
		resultPath += "/";
	return Utils::ReplaceAll(resultPath, "\\", "/");
}
