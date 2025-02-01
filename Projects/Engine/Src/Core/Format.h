#pragma once

namespace RS
{
	enum Format : uint32
	{
		RS_FORMAT_UNKNOWN = 0,

        // 32 bit depth
        RS_FORMAT_R32G32B32A32_TYPELESS,
        RS_FORMAT_R32G32B32A32_FLOAT,
        RS_FORMAT_R32G32B32A32_UINT,
        RS_FORMAT_R32G32B32A32_SINT,
        RS_FORMAT_R32G32B32_TYPELESS,
        RS_FORMAT_R32G32B32_FLOAT,
        RS_FORMAT_R32G32B32_UINT,
        RS_FORMAT_R32G32B32_SINT,
        RS_FORMAT_R32G32_TYPELESS,
        RS_FORMAT_R32G32_FLOAT,
        RS_FORMAT_R32G32_UINT,
        RS_FORMAT_R32G32_SINT,
        RS_FORMAT_R32_TYPELESS,
        RS_FORMAT_R32_FLOAT,
        RS_FORMAT_R32_UINT,
        RS_FORMAT_R32_SINT,

        // 16 bit depth
        RS_FORMAT_R16G16B16A16_TYPELESS,
        RS_FORMAT_R16G16B16A16_FLOAT,
        RS_FORMAT_R16G16B16A16_UNORM,
        RS_FORMAT_R16G16B16A16_SNORM,
        RS_FORMAT_R16G16B16A16_UINT,
        RS_FORMAT_R16G16B16A16_SINT,
        RS_FORMAT_R16G16_TYPELESS,
        RS_FORMAT_R16G16_FLOAT,
        RS_FORMAT_R16G16_UNORM,
        RS_FORMAT_R16G16_SNORM,
        RS_FORMAT_R16G16_UINT,
        RS_FORMAT_R16G16_SINT,
        RS_FORMAT_R16_TYPELESS,
        RS_FORMAT_R16_FLOAT,
        RS_FORMAT_R16_UNORM,
        RS_FORMAT_R16_SNORM,
        RS_FORMAT_R16_UINT,
        RS_FORMAT_R16_SINT,

        // 8 bit depth
		RS_FORMAT_R8G8B8A8_TYPELESS,
		RS_FORMAT_R8G8B8A8_UNORM,
		RS_FORMAT_R8G8B8A8_UNORM_SRGB,
		RS_FORMAT_R8G8B8A8_UINT,
		RS_FORMAT_R8G8B8A8_SNORM,
		RS_FORMAT_R8G8B8A8_SINT,
		RS_FORMAT_R8G8_TYPELESS,
        RS_FORMAT_R8G8_UNORM,
		RS_FORMAT_R8G8_UINT,
		RS_FORMAT_R8G8_SNORM,
		RS_FORMAT_R8G8_SINT,
		RS_FORMAT_R8_TYPELESS,
		RS_FORMAT_R8_UNORM,
		RS_FORMAT_R8_UINT,
		RS_FORMAT_R8_SNORM,
		RS_FORMAT_R8_SINT,
	};

	enum class FormatType
	{
		TYPELESS,
		UNORM,
		UINT,
		SNORM,
		SINT,
		FLOAT,
		UNORM_SRGB
	};

    struct FormatInfo
    {
        uint32 channelCount;
        uint32 bitDepth;
        FormatType formatType;
    };

    constexpr uint32 GetChannelCountFromFormat(Format format)
	{
		switch (format)
		{
			case RS_FORMAT_R32G32B32A32_TYPELESS:
			case RS_FORMAT_R32G32B32A32_FLOAT:
			case RS_FORMAT_R32G32B32A32_UINT:
			case RS_FORMAT_R32G32B32A32_SINT:
			case RS_FORMAT_R16G16B16A16_TYPELESS:
			case RS_FORMAT_R16G16B16A16_FLOAT:
			case RS_FORMAT_R16G16B16A16_UNORM:
			case RS_FORMAT_R16G16B16A16_SNORM:
			case RS_FORMAT_R16G16B16A16_UINT:
			case RS_FORMAT_R16G16B16A16_SINT:
			case RS_FORMAT_R8G8B8A8_TYPELESS:
			case RS_FORMAT_R8G8B8A8_UNORM:
			case RS_FORMAT_R8G8B8A8_UNORM_SRGB:
			case RS_FORMAT_R8G8B8A8_UINT:
			case RS_FORMAT_R8G8B8A8_SNORM:
			case RS_FORMAT_R8G8B8A8_SINT:
				return 4;
			case RS_FORMAT_R32G32B32_TYPELESS:
			case RS_FORMAT_R32G32B32_FLOAT:
			case RS_FORMAT_R32G32B32_UINT:
			case RS_FORMAT_R32G32B32_SINT:
				return 3;
			case RS_FORMAT_R32G32_TYPELESS:
			case RS_FORMAT_R32G32_FLOAT:
			case RS_FORMAT_R32G32_UINT:
			case RS_FORMAT_R32G32_SINT:
			case RS_FORMAT_R16G16_TYPELESS:
			case RS_FORMAT_R16G16_FLOAT:
			case RS_FORMAT_R16G16_UNORM:
			case RS_FORMAT_R16G16_SNORM:
			case RS_FORMAT_R16G16_UINT:
			case RS_FORMAT_R16G16_SINT:
			case RS_FORMAT_R8G8_TYPELESS:
			case RS_FORMAT_R8G8_UNORM:
			case RS_FORMAT_R8G8_UINT:
			case RS_FORMAT_R8G8_SNORM:
			case RS_FORMAT_R8G8_SINT:
				return 2;
			case RS_FORMAT_R32_TYPELESS:
			case RS_FORMAT_R32_FLOAT:
			case RS_FORMAT_R32_UINT:
			case RS_FORMAT_R32_SINT:
			case RS_FORMAT_R16_TYPELESS:
			case RS_FORMAT_R16_FLOAT:
			case RS_FORMAT_R16_UNORM:
			case RS_FORMAT_R16_SNORM:
			case RS_FORMAT_R16_UINT:
			case RS_FORMAT_R16_SINT:
			case RS_FORMAT_R8_TYPELESS:
			case RS_FORMAT_R8_UNORM:
			case RS_FORMAT_R8_UINT:
			case RS_FORMAT_R8_SNORM:
			case RS_FORMAT_R8_SINT:
				return 1;
			default:
				RS_ASSERT(false, "Cannot get number of channels from this format. Format: {}", (uint32)format);
				break;
		}
	}

    constexpr FormatInfo GetFormatInfo(Format format)
    {
        RS_ASSERT(format != RS_FORMAT_UNKNOWN);

        auto CreateFormatIinfo = [](uint32 channelCount, uint32 bitDepth, FormatType formatType)->FormatInfo
        {
            FormatInfo info;
            info.channelCount = channelCount;
            info.bitDepth = bitDepth;
            info.formatType = formatType;
            return info;
        };

        switch (format)
        {
        case RS::RS_FORMAT_R32G32B32A32_TYPELESS:   return CreateFormatIinfo(4, 32, FormatType::TYPELESS);
        case RS::RS_FORMAT_R32G32B32A32_FLOAT:      return CreateFormatIinfo(4, 32, FormatType::FLOAT);
        case RS::RS_FORMAT_R32G32B32A32_UINT:       return CreateFormatIinfo(4, 32, FormatType::UINT);
        case RS::RS_FORMAT_R32G32B32A32_SINT:       return CreateFormatIinfo(4, 32, FormatType::SINT);
        case RS::RS_FORMAT_R32G32B32_TYPELESS:      return CreateFormatIinfo(3, 32, FormatType::TYPELESS);
        case RS::RS_FORMAT_R32G32B32_FLOAT:         return CreateFormatIinfo(3, 32, FormatType::FLOAT);
        case RS::RS_FORMAT_R32G32B32_UINT:          return CreateFormatIinfo(3, 32, FormatType::UINT);
        case RS::RS_FORMAT_R32G32B32_SINT:          return CreateFormatIinfo(3, 32, FormatType::SINT);
        case RS::RS_FORMAT_R32G32_TYPELESS:         return CreateFormatIinfo(2, 32, FormatType::TYPELESS);
        case RS::RS_FORMAT_R32G32_FLOAT:            return CreateFormatIinfo(2, 32, FormatType::FLOAT);
        case RS::RS_FORMAT_R32G32_UINT:             return CreateFormatIinfo(2, 32, FormatType::UINT);
        case RS::RS_FORMAT_R32G32_SINT:             return CreateFormatIinfo(2, 32, FormatType::SINT);
        case RS::RS_FORMAT_R32_TYPELESS:            return CreateFormatIinfo(1, 32, FormatType::TYPELESS);
        case RS::RS_FORMAT_R32_FLOAT:               return CreateFormatIinfo(1, 32, FormatType::FLOAT);
        case RS::RS_FORMAT_R32_UINT:                return CreateFormatIinfo(1, 32, FormatType::UINT);
        case RS::RS_FORMAT_R32_SINT:                return CreateFormatIinfo(1, 32, FormatType::SINT);
        case RS::RS_FORMAT_R16G16B16A16_TYPELESS:   return CreateFormatIinfo(4, 16, FormatType::TYPELESS);
        case RS::RS_FORMAT_R16G16B16A16_FLOAT:      return CreateFormatIinfo(4, 16, FormatType::FLOAT);
        case RS::RS_FORMAT_R16G16B16A16_UNORM:      return CreateFormatIinfo(4, 16, FormatType::UNORM);
        case RS::RS_FORMAT_R16G16B16A16_SNORM:      return CreateFormatIinfo(4, 16, FormatType::SNORM);
        case RS::RS_FORMAT_R16G16B16A16_UINT:       return CreateFormatIinfo(4, 16, FormatType::UINT);
        case RS::RS_FORMAT_R16G16B16A16_SINT:       return CreateFormatIinfo(4, 16, FormatType::SINT);
        case RS::RS_FORMAT_R16G16_TYPELESS:         return CreateFormatIinfo(2, 16, FormatType::TYPELESS);
        case RS::RS_FORMAT_R16G16_FLOAT:            return CreateFormatIinfo(2, 16, FormatType::FLOAT);
        case RS::RS_FORMAT_R16G16_UNORM:            return CreateFormatIinfo(2, 16, FormatType::UNORM);
        case RS::RS_FORMAT_R16G16_SNORM:            return CreateFormatIinfo(2, 16, FormatType::SNORM);
        case RS::RS_FORMAT_R16G16_UINT:             return CreateFormatIinfo(2, 16, FormatType::UINT);
        case RS::RS_FORMAT_R16G16_SINT:             return CreateFormatIinfo(2, 16, FormatType::SINT);
        case RS::RS_FORMAT_R16_TYPELESS:            return CreateFormatIinfo(1, 16, FormatType::TYPELESS);
        case RS::RS_FORMAT_R16_FLOAT:               return CreateFormatIinfo(1, 16, FormatType::FLOAT);
        case RS::RS_FORMAT_R16_UNORM:               return CreateFormatIinfo(1, 16, FormatType::UNORM);
        case RS::RS_FORMAT_R16_SNORM:               return CreateFormatIinfo(1, 16, FormatType::SNORM);
        case RS::RS_FORMAT_R16_UINT:                return CreateFormatIinfo(1, 16, FormatType::UINT);
        case RS::RS_FORMAT_R16_SINT:                return CreateFormatIinfo(1, 16, FormatType::SINT);
        case RS::RS_FORMAT_R8G8B8A8_TYPELESS:       return CreateFormatIinfo(4, 8, FormatType::TYPELESS);
        case RS::RS_FORMAT_R8G8B8A8_UNORM:          return CreateFormatIinfo(4, 8, FormatType::UNORM);
        case RS::RS_FORMAT_R8G8B8A8_UNORM_SRGB:     return CreateFormatIinfo(4, 8, FormatType::UNORM_SRGB);
        case RS::RS_FORMAT_R8G8B8A8_UINT:           return CreateFormatIinfo(4, 8, FormatType::UINT);
        case RS::RS_FORMAT_R8G8B8A8_SNORM:          return CreateFormatIinfo(4, 8, FormatType::SNORM);
        case RS::RS_FORMAT_R8G8B8A8_SINT:           return CreateFormatIinfo(4, 8, FormatType::SINT);
        case RS::RS_FORMAT_R8G8_TYPELESS:           return CreateFormatIinfo(2, 8, FormatType::TYPELESS);
        case RS::RS_FORMAT_R8G8_UNORM:              return CreateFormatIinfo(2, 8, FormatType::UNORM);
        case RS::RS_FORMAT_R8G8_UINT:               return CreateFormatIinfo(2, 8, FormatType::UINT);
        case RS::RS_FORMAT_R8G8_SNORM:              return CreateFormatIinfo(2, 8, FormatType::SNORM);
        case RS::RS_FORMAT_R8G8_SINT:               return CreateFormatIinfo(2, 8, FormatType::SINT);
        case RS::RS_FORMAT_R8_TYPELESS:             return CreateFormatIinfo(1, 8, FormatType::TYPELESS);
        case RS::RS_FORMAT_R8_UNORM:                return CreateFormatIinfo(1, 8, FormatType::UNORM);
        case RS::RS_FORMAT_R8_UINT:                 return CreateFormatIinfo(1, 8, FormatType::UINT);
        case RS::RS_FORMAT_R8_SNORM:                return CreateFormatIinfo(1, 8, FormatType::SNORM);
        case RS::RS_FORMAT_R8_SINT:                 return CreateFormatIinfo(1, 8, FormatType::SINT);
        default:
            RS_ASSERT(false, "Unknown Format!");
            return {};
            break;
        }
    }

	constexpr Format GetFormat(uint32 channelCount, uint32 bitDepth, FormatType type)
	{
		RS_ASSERT(channelCount <= 4);
		RS_ASSERT(bitDepth == 8 || bitDepth == 16 || bitDepth == 32);

        switch (type)
        {
        case FormatType::TYPELESS:
        {
            if (bitDepth == 32)
            {
                switch (channelCount)
                {
                case 4: return RS_FORMAT_R32G32B32A32_TYPELESS;
                case 3: return RS_FORMAT_R32G32B32_TYPELESS;
                case 2: return RS_FORMAT_R32G32_TYPELESS;
                case 1: return RS_FORMAT_R32_TYPELESS;
                default:
                    RS_ASSERT(false, "TYPELESS 32 bit does not support {} channels!", channelCount);
                    break;
                }
            }
            else if (bitDepth == 16)
            {
                switch (channelCount)
                {
                case 4: return RS_FORMAT_R16G16B16A16_TYPELESS;
                case 2: return RS_FORMAT_R16G16_TYPELESS;
                case 1: return RS_FORMAT_R16_TYPELESS;
                default:
                    RS_ASSERT(false, "TYPELESS 16 bit does not support {} channels!", channelCount);
                    break;
                }
            }
            else if (bitDepth == 8)
            {
                switch (channelCount)
                {
                case 4: return RS_FORMAT_R8G8B8A8_TYPELESS;
                case 2: return RS_FORMAT_R8G8_TYPELESS;
                case 1: return RS_FORMAT_R8_TYPELESS;
                default:
                    RS_ASSERT(false, "TYPELESS 8 bit does not support {} channels!", channelCount);
                    break;
                }
            }
            else
                RS_ASSERT(false, "Channel size is not supported for TYPELESS formats! bitDepth:", bitDepth);
            break;
        }
        case FormatType::UNORM:
        {
            if (bitDepth == 32)
            {
                RS_ASSERT(false, "UNORM 32 bit does not support {} channels!", channelCount);
            }
            else if (bitDepth == 16)
            {
                switch (channelCount)
                {
                case 4: return RS_FORMAT_R16G16B16A16_UNORM;
                case 2: return RS_FORMAT_R16G16_UNORM;
                case 1: return RS_FORMAT_R16_UNORM;
                default:
                    RS_ASSERT(false, "UNORM 16 bit does not support {} channels!", channelCount);
                    break;
                }
            }
            else if (bitDepth == 8)
            {
                switch (channelCount)
                {
                case 4: return RS_FORMAT_R8G8B8A8_UNORM;
                case 2: return RS_FORMAT_R8G8_UNORM;
                case 1: return RS_FORMAT_R8_UNORM;
                default:
                    RS_ASSERT(false, "UNORM 8 bit does not support {} channels!", channelCount);
                    break;
                }
            }
            else
                RS_ASSERT(false, "Channel size is not supported for UNORM formats! bitDepth:", bitDepth);
            break;
        }
        case FormatType::UINT:
        {
            if (bitDepth == 32)
            {
                switch (channelCount)
                {
                case 4: return RS_FORMAT_R32G32B32A32_UINT;
                case 3: return RS_FORMAT_R32G32B32_UINT;
                case 2: return RS_FORMAT_R32G32_UINT;
                case 1: return RS_FORMAT_R32_UINT;
                default:
                    RS_ASSERT(false, "UINT 32 bit does not support {} channels!", channelCount);
                    break;
                }
            }
            else if (bitDepth == 16)
            {
                switch (channelCount)
                {
                case 4: return RS_FORMAT_R16G16B16A16_UINT;
                case 2: return RS_FORMAT_R16G16_UINT;
                case 1: return RS_FORMAT_R16_UINT;
                default:
                    RS_ASSERT(false, "UINT 16 bit does not support {} channels!", channelCount);
                    break;
                }
            }
            else if (bitDepth == 8)
            {
                switch (channelCount)
                {
                case 4: return RS_FORMAT_R8G8B8A8_UINT;
                case 2: return RS_FORMAT_R8G8_UINT;
                case 1: return RS_FORMAT_R8_UINT;
                default:
                    RS_ASSERT(false, "UINT 8 bit does not support {} channels!", channelCount);
                    break;
                }
            }
            else
                RS_ASSERT(false, "Channel size is not supported for UINT formats! bitDepth:", bitDepth);
            break;
        }
        case FormatType::SNORM:
        {
            if (bitDepth == 32)
            {
                RS_ASSERT(false, "SNORM 32 bit does not support {} channels!", channelCount);
            }
            else if (bitDepth == 16)
            {
                switch (channelCount)
                {
                case 4: return RS_FORMAT_R16G16B16A16_SNORM;
                case 2: return RS_FORMAT_R16G16_SNORM;
                case 1: return RS_FORMAT_R16_SNORM;
                default:
                    RS_ASSERT(false, "SNORM 16 bit does not support {} channels!", channelCount);
                    break;
                }
            }
            else if (bitDepth == 8)
            {
                switch (channelCount)
                {
                case 4: return RS_FORMAT_R8G8B8A8_SNORM;
                case 2: return RS_FORMAT_R8G8_SNORM;
                case 1: return RS_FORMAT_R8_SNORM;
                default:
                    RS_ASSERT(false, "SNORM 8 bit does not support {} channels!", channelCount);
                    break;
                }
            }
            else
                RS_ASSERT(false, "Channel size is not supported for SNORM formats! bitDepth:", bitDepth);
            break;
        }
        case FormatType::SINT:
        {
            if (bitDepth == 32)
            {
                switch (channelCount)
                {
                case 4: return RS_FORMAT_R32G32B32A32_SINT;
                case 3: return RS_FORMAT_R32G32B32_SINT;
                case 2: return RS_FORMAT_R32G32_SINT;
                case 1: return RS_FORMAT_R32_SINT;
                default:
                    RS_ASSERT(false, "SINT 32 bit does not support {} channels!", channelCount);
                    break;
                }
            }
            else if (bitDepth == 16)
            {
                switch (channelCount)
                {
                case 4: return RS_FORMAT_R16G16B16A16_SINT;
                case 2: return RS_FORMAT_R16G16_SINT;
                case 1: return RS_FORMAT_R16_SINT;
                default:
                    RS_ASSERT(false, "SINT 16 bit does not support {} channels!", channelCount);
                    break;
                }
            }
            else if (bitDepth == 8)
            {
                switch (channelCount)
                {
                case 4: return RS_FORMAT_R8G8B8A8_SINT;
                case 2: return RS_FORMAT_R8G8_SINT;
                case 1: return RS_FORMAT_R8_SINT;
                default:
                    RS_ASSERT(false, "SINT 8 bit does not support {} channels!", channelCount);
                    break;
                }
            }
            else
                RS_ASSERT(false, "Channel size is not supported for SINT formats! bitDepth:", bitDepth);
            break;
        }
        case FormatType::FLOAT:
        {
            if (bitDepth == 32)
            {
                switch (channelCount)
                {
                case 4: return RS_FORMAT_R32G32B32A32_FLOAT;
                case 3: return RS_FORMAT_R32G32B32_FLOAT;
                case 2: return RS_FORMAT_R32G32_FLOAT;
                case 1: return RS_FORMAT_R32_FLOAT;
                default:
                    RS_ASSERT(false, "FLOAT 32 bit does not support {} channels!", channelCount);
                    break;
                }
            }
            else if (bitDepth == 16)
            {
                switch (channelCount)
                {
                case 4: return RS_FORMAT_R16G16B16A16_FLOAT;
                case 2: return RS_FORMAT_R16G16_FLOAT;
                case 1: return RS_FORMAT_R16_FLOAT;
                default:
                    RS_ASSERT(false, "FLOAT 16 bit does not support {} channels!", channelCount);
                    break;
                }
            }
            else if (bitDepth == 8)
            {
                RS_ASSERT(false, "FLOAT 8 bit does not support {} channels!", channelCount);
            }
            else
                RS_ASSERT(false, "Channel size is not supported for FLOAT formats! bitDepth:", bitDepth);
            break;
        }
        case FormatType::UNORM_SRGB:
        {
            if (bitDepth == 32)
            {
                RS_ASSERT(false, "UNORM_SRGB 32 bit does not support {} channels!", channelCount);
            }
            else if (bitDepth == 16)
            {
                RS_ASSERT(false, "UNORM_SRGB 16 bit does not support {} channels!", channelCount);
            }
            else if (bitDepth == 8)
            {
                switch (channelCount)
                {
                case 4: return RS_FORMAT_R8G8B8A8_UNORM_SRGB;
                default:
                    RS_ASSERT(false, "UNORM_SRGB 8 bit does not support {} channels!", channelCount);
                    break;
                }
            }
            else
                RS_ASSERT(false, "Channel size is not supported for UNORM_SRGB formats! bitDepth:", bitDepth);
            break;
        }
        default:
            RS_ASSERT(false, "Type cannot be converted to DXGI_Format! Type:", (uint32)type);
            break;
        }
        return RS_FORMAT_UNKNOWN;
	}

    constexpr Format GetFormat(const FormatInfo& info)
    {
        return GetFormat(info.channelCount, info.bitDepth, info.formatType);
    }
}
