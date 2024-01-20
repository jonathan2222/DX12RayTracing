#pragma once

#include "Format.h"

namespace RS
{
	class CorePlatform
	{
	public:
		RS_DEFAULT_ABSTRACT_CLASS(CorePlatform)

		static std::shared_ptr<CorePlatform> Get();

		std::string GetComputerNameStr() const;
		std::string GetUserNameStr() const;

		std::string GetConfigurationAsStr() const;

		RS_BEGIN_FLAGS_U32(ImageFlag)
			RS_FLAG(FLIP_Y)
			RS_END_FLAGS()
		struct Image
		{
			Image() : pData(nullptr), width(0u), height(0u), format(RS_FORMAT_UNKNOWN) {}
			virtual ~Image();
			uint8* pData;
			uint32 width;
			uint32 height;
			Format format;
		};
		std::unique_ptr<Image> LoadImageData(const std::string& path, Format requestedFormat, ImageFlags flags = ImageFlag::NONE);

		struct BinaryFile
		{
			std::unique_ptr<uint8> pData = nullptr;
			uint64 size = 0;
		};
		static BinaryFile LoadBinaryFile(const std::string& path, uint32 offset = 0);

		static std::string GetLastErrorString();
	};
}