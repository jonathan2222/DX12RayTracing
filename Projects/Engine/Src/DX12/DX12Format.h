#pragma once

#include "d3dx12.h"

#include "Core/Format.h"

namespace RS::DX12
{
    constexpr uint32 GetChannelCountFromFormat(DXGI_FORMAT format)
    {
        switch (format)
        {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_B5G5R5A1_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B4G4R4A4_UNORM:
            return 4;
            break;
        case DXGI_FORMAT_R32G32B32_TYPELESS:
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
        case DXGI_FORMAT_R11G11B10_FLOAT:
        case DXGI_FORMAT_B5G6R5_UNORM:
            return 3;
            break;
        case DXGI_FORMAT_R32G32_TYPELESS:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_R16G16_TYPELESS:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_R8G8_TYPELESS:
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_SINT:
            return 2;
            break;
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_A8_UNORM:
        case DXGI_FORMAT_R1_UNORM:
            return 1;
            break;
        default:
            RS_ASSERT(false, "Cannot get number of channels from this format. Format: {}", (uint32)format);
            break;
        }
    }

    constexpr FormatInfo GetFormatInfo(DXGI_FORMAT format)
    {
        RS_ASSERT_NO_MSG(format != RS_FORMAT_UNKNOWN);

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
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:   return CreateFormatIinfo(4, 32, FormatType::TYPELESS);
        case DXGI_FORMAT_R32G32B32A32_FLOAT:      return CreateFormatIinfo(4, 32, FormatType::FLOAT);
        case DXGI_FORMAT_R32G32B32A32_UINT:       return CreateFormatIinfo(4, 32, FormatType::UINT);
        case DXGI_FORMAT_R32G32B32A32_SINT:       return CreateFormatIinfo(4, 32, FormatType::SINT);
        case DXGI_FORMAT_R32G32B32_TYPELESS:      return CreateFormatIinfo(3, 32, FormatType::TYPELESS);
        case DXGI_FORMAT_R32G32B32_FLOAT:         return CreateFormatIinfo(3, 32, FormatType::FLOAT);
        case DXGI_FORMAT_R32G32B32_UINT:          return CreateFormatIinfo(3, 32, FormatType::UINT);
        case DXGI_FORMAT_R32G32B32_SINT:          return CreateFormatIinfo(3, 32, FormatType::SINT);
        case DXGI_FORMAT_R32G32_TYPELESS:         return CreateFormatIinfo(2, 32, FormatType::TYPELESS);
        case DXGI_FORMAT_R32G32_FLOAT:            return CreateFormatIinfo(2, 32, FormatType::FLOAT);
        case DXGI_FORMAT_R32G32_UINT:             return CreateFormatIinfo(2, 32, FormatType::UINT);
        case DXGI_FORMAT_R32G32_SINT:             return CreateFormatIinfo(2, 32, FormatType::SINT);
        case DXGI_FORMAT_R32_TYPELESS:            return CreateFormatIinfo(1, 32, FormatType::TYPELESS);
        case DXGI_FORMAT_R32_FLOAT:               return CreateFormatIinfo(1, 32, FormatType::FLOAT);
        case DXGI_FORMAT_R32_UINT:                return CreateFormatIinfo(1, 32, FormatType::UINT);
        case DXGI_FORMAT_R32_SINT:                return CreateFormatIinfo(1, 32, FormatType::SINT);
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:   return CreateFormatIinfo(4, 16, FormatType::TYPELESS);
        case DXGI_FORMAT_R16G16B16A16_FLOAT:      return CreateFormatIinfo(4, 16, FormatType::FLOAT);
        case DXGI_FORMAT_R16G16B16A16_UNORM:      return CreateFormatIinfo(4, 16, FormatType::UNORM);
        case DXGI_FORMAT_R16G16B16A16_SNORM:      return CreateFormatIinfo(4, 16, FormatType::SNORM);
        case DXGI_FORMAT_R16G16B16A16_UINT:       return CreateFormatIinfo(4, 16, FormatType::UINT);
        case DXGI_FORMAT_R16G16B16A16_SINT:       return CreateFormatIinfo(4, 16, FormatType::SINT);
        case DXGI_FORMAT_R16G16_TYPELESS:         return CreateFormatIinfo(2, 16, FormatType::TYPELESS);
        case DXGI_FORMAT_R16G16_FLOAT:            return CreateFormatIinfo(2, 16, FormatType::FLOAT);
        case DXGI_FORMAT_R16G16_UNORM:            return CreateFormatIinfo(2, 16, FormatType::UNORM);
        case DXGI_FORMAT_R16G16_SNORM:            return CreateFormatIinfo(2, 16, FormatType::SNORM);
        case DXGI_FORMAT_R16G16_UINT:             return CreateFormatIinfo(2, 16, FormatType::UINT);
        case DXGI_FORMAT_R16G16_SINT:             return CreateFormatIinfo(2, 16, FormatType::SINT);
        case DXGI_FORMAT_R16_TYPELESS:            return CreateFormatIinfo(1, 16, FormatType::TYPELESS);
        case DXGI_FORMAT_R16_FLOAT:               return CreateFormatIinfo(1, 16, FormatType::FLOAT);
        case DXGI_FORMAT_R16_UNORM:               return CreateFormatIinfo(1, 16, FormatType::UNORM);
        case DXGI_FORMAT_R16_SNORM:               return CreateFormatIinfo(1, 16, FormatType::SNORM);
        case DXGI_FORMAT_R16_UINT:                return CreateFormatIinfo(1, 16, FormatType::UINT);
        case DXGI_FORMAT_R16_SINT:                return CreateFormatIinfo(1, 16, FormatType::SINT);
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:       return CreateFormatIinfo(4, 8, FormatType::TYPELESS);
        case DXGI_FORMAT_R8G8B8A8_UNORM:          return CreateFormatIinfo(4, 8, FormatType::UNORM);
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:     return CreateFormatIinfo(4, 8, FormatType::UNORM_SRGB);
        case DXGI_FORMAT_R8G8B8A8_UINT:           return CreateFormatIinfo(4, 8, FormatType::UINT);
        case DXGI_FORMAT_R8G8B8A8_SNORM:          return CreateFormatIinfo(4, 8, FormatType::SNORM);
        case DXGI_FORMAT_R8G8B8A8_SINT:           return CreateFormatIinfo(4, 8, FormatType::SINT);
        case DXGI_FORMAT_R8G8_TYPELESS:           return CreateFormatIinfo(2, 8, FormatType::TYPELESS);
        case DXGI_FORMAT_R8G8_UNORM:              return CreateFormatIinfo(2, 8, FormatType::UNORM);
        case DXGI_FORMAT_R8G8_UINT:               return CreateFormatIinfo(2, 8, FormatType::UINT);
        case DXGI_FORMAT_R8G8_SNORM:              return CreateFormatIinfo(2, 8, FormatType::SNORM);
        case DXGI_FORMAT_R8G8_SINT:               return CreateFormatIinfo(2, 8, FormatType::SINT);
        case DXGI_FORMAT_R8_TYPELESS:             return CreateFormatIinfo(1, 8, FormatType::TYPELESS);
        case DXGI_FORMAT_R8_UNORM:                return CreateFormatIinfo(1, 8, FormatType::UNORM);
        case DXGI_FORMAT_R8_UINT:                 return CreateFormatIinfo(1, 8, FormatType::UINT);
        case DXGI_FORMAT_R8_SNORM:                return CreateFormatIinfo(1, 8, FormatType::SNORM);
        case DXGI_FORMAT_R8_SINT:                 return CreateFormatIinfo(1, 8, FormatType::SINT);
        default:
            RS_ASSERT(false, "Unknown Format!");
            return {};
            break;
        }
    }

    constexpr DXGI_FORMAT GetDXGIFormat(uint8 numChannels, uint8 channelSize, FormatType type)
    {
        switch (type)
        {
        case RS::FormatType::TYPELESS:
        {
            if (channelSize == 32)
            {
                switch (numChannels)
                {
                case 4: return DXGI_FORMAT_R32G32B32A32_TYPELESS;
                case 3: return DXGI_FORMAT_R32G32B32_TYPELESS;
                case 2: return DXGI_FORMAT_R32G32_TYPELESS;
                case 1: return DXGI_FORMAT_R32_TYPELESS;
                default:
                    RS_ASSERT(false, "TYPELESS 32 bit does not support {} channels!", numChannels);
                    break;
                }
            }
            else if (channelSize == 16)
            {
                switch (numChannels)
                {
                case 4: return DXGI_FORMAT_R16G16B16A16_TYPELESS;
                case 2: return DXGI_FORMAT_R16G16_TYPELESS;
                case 1: return DXGI_FORMAT_R16_TYPELESS;
                default:
                    RS_ASSERT(false, "TYPELESS 16 bit does not support {} channels!", numChannels);
                    break;
                }
            }
            else if (channelSize == 8)
            {
                switch (numChannels)
                {
                case 4: return DXGI_FORMAT_R8G8B8A8_TYPELESS;
                case 2: return DXGI_FORMAT_R8G8_TYPELESS;
                case 1: return DXGI_FORMAT_R8_TYPELESS;
                default:
                    RS_ASSERT(false, "TYPELESS 8 bit does not support {} channels!", numChannels);
                    break;
                }
            }
            else
                RS_ASSERT(false, "Channel size is not supported for TYPELESS formats! ChannelSize:", channelSize);
            break;
        }
        case RS::FormatType::UNORM:
        {
            if (channelSize == 32)
            {
                RS_ASSERT(false, "UNORM 32 bit does not support {} channels!", numChannels);
            }
            else if (channelSize == 16)
            {
                switch (numChannels)
                {
                case 4: return DXGI_FORMAT_R16G16B16A16_UNORM;
                case 2: return DXGI_FORMAT_R16G16_UNORM;
                case 1: return DXGI_FORMAT_R16_UNORM;
                default:
                    RS_ASSERT(false, "UNORM 16 bit does not support {} channels!", numChannels);
                    break;
                }
            }
            else if (channelSize == 8)
            {
                switch (numChannels)
                {
                case 4: return DXGI_FORMAT_R8G8B8A8_UNORM;
                case 2: return DXGI_FORMAT_R8G8_UNORM;
                case 1: return DXGI_FORMAT_R8_UNORM;
                default:
                    RS_ASSERT(false, "UNORM 8 bit does not support {} channels!", numChannels);
                    break;
                }
            }
            else
                RS_ASSERT(false, "Channel size is not supported for UNORM formats! ChannelSize:", channelSize);
            break;
        }
        case RS::FormatType::UINT:
        {
            if (channelSize == 32)
            {
                switch (numChannels)
                {
                case 4: return DXGI_FORMAT_R32G32B32A32_UINT;
                case 3: return DXGI_FORMAT_R32G32B32_UINT;
                case 2: return DXGI_FORMAT_R32G32_UINT;
                case 1: return DXGI_FORMAT_R32_UINT;
                default:
                    RS_ASSERT(false, "UINT 32 bit does not support {} channels!", numChannels);
                    break;
                }
            }
            else if (channelSize == 16)
            {
                switch (numChannels)
                {
                case 4: return DXGI_FORMAT_R16G16B16A16_UINT;
                case 2: return DXGI_FORMAT_R16G16_UINT;
                case 1: return DXGI_FORMAT_R16_UINT;
                default:
                    RS_ASSERT(false, "UINT 16 bit does not support {} channels!", numChannels);
                    break;
                }
            }
            else if (channelSize == 8)
            {
                switch (numChannels)
                {
                case 4: return DXGI_FORMAT_R8G8B8A8_UINT;
                case 2: return DXGI_FORMAT_R8G8_UINT;
                case 1: return DXGI_FORMAT_R8_UINT;
                default:
                    RS_ASSERT(false, "UINT 8 bit does not support {} channels!", numChannels);
                    break;
                }
            }
            else
                RS_ASSERT(false, "Channel size is not supported for UINT formats! ChannelSize:", channelSize);
            break;
        }
        case RS::FormatType::SNORM:
        {
            if (channelSize == 32)
            {
                RS_ASSERT(false, "SNORM 32 bit does not support {} channels!", numChannels);
            }
            else if (channelSize == 16)
            {
                switch (numChannels)
                {
                case 4: return DXGI_FORMAT_R16G16B16A16_SNORM;
                case 2: return DXGI_FORMAT_R16G16_SNORM;
                case 1: return DXGI_FORMAT_R16_SNORM;
                default:
                    RS_ASSERT(false, "SNORM 16 bit does not support {} channels!", numChannels);
                    break;
                }
            }
            else if (channelSize == 8)
            {
                switch (numChannels)
                {
                case 4: return DXGI_FORMAT_R8G8B8A8_SNORM;
                case 2: return DXGI_FORMAT_R8G8_SNORM;
                case 1: return DXGI_FORMAT_R8_SNORM;
                default:
                    RS_ASSERT(false, "SNORM 8 bit does not support {} channels!", numChannels);
                    break;
                }
            }
            else
                RS_ASSERT(false, "Channel size is not supported for SNORM formats! ChannelSize:", channelSize);
            break;
        }
        case RS::FormatType::SINT:
        {
            if (channelSize == 32)
            {
                switch (numChannels)
                {
                case 4: return DXGI_FORMAT_R32G32B32A32_SINT;
                case 3: return DXGI_FORMAT_R32G32B32_SINT;
                case 2: return DXGI_FORMAT_R32G32_SINT;
                case 1: return DXGI_FORMAT_R32_SINT;
                default:
                    RS_ASSERT(false, "SINT 32 bit does not support {} channels!", numChannels);
                    break;
                }
            }
            else if (channelSize == 16)
            {
                switch (numChannels)
                {
                case 4: return DXGI_FORMAT_R16G16B16A16_SINT;
                case 2: return DXGI_FORMAT_R16G16_SINT;
                case 1: return DXGI_FORMAT_R16_SINT;
                default:
                    RS_ASSERT(false, "SINT 16 bit does not support {} channels!", numChannels);
                    break;
                }
            }
            else if (channelSize == 8)
            {
                switch (numChannels)
                {
                case 4: return DXGI_FORMAT_R8G8B8A8_SINT;
                case 2: return DXGI_FORMAT_R8G8_SINT;
                case 1: return DXGI_FORMAT_R8_SINT;
                default:
                    RS_ASSERT(false, "SINT 8 bit does not support {} channels!", numChannels);
                    break;
                }
            }
            else
                RS_ASSERT(false, "Channel size is not supported for SINT formats! ChannelSize:", channelSize);
            break;
        }
        case RS::FormatType::FLOAT:
        {
            if (channelSize == 32)
            {
                switch (numChannels)
                {
                case 4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
                case 3: return DXGI_FORMAT_R32G32B32_FLOAT;
                case 2: return DXGI_FORMAT_R32G32_FLOAT;
                case 1: return DXGI_FORMAT_R32_FLOAT;
                default:
                    RS_ASSERT(false, "FLOAT 32 bit does not support {} channels!", numChannels);
                    break;
                }
            }
            else if (channelSize == 16)
            {
                switch (numChannels)
                {
                case 4: return DXGI_FORMAT_R16G16B16A16_FLOAT;
                case 2: return DXGI_FORMAT_R16G16_FLOAT;
                case 1: return DXGI_FORMAT_R16_FLOAT;
                default:
                    RS_ASSERT(false, "FLOAT 16 bit does not support {} channels!", numChannels);
                    break;
                }
            }
            else if (channelSize == 8)
            {
                RS_ASSERT(false, "FLOAT 8 bit does not support {} channels!", numChannels);
            }
            else
                RS_ASSERT(false, "Channel size is not supported for FLOAT formats! ChannelSize:", channelSize);
            break;
        }
        case RS::FormatType::UNORM_SRGB:
        {
            if (channelSize == 32)
            {
                RS_ASSERT(false, "UNORM_SRGB 32 bit does not support {} channels!", numChannels);
            }
            else if (channelSize == 16)
            {
                RS_ASSERT(false, "UNORM_SRGB 16 bit does not support {} channels!", numChannels);
            }
            else if (channelSize == 8)
            {
                switch (numChannels)
                {
                case 4: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
                default:
                    RS_ASSERT(false, "UNORM_SRGB 8 bit does not support {} channels!", numChannels);
                    break;
                }
            }
            else
                RS_ASSERT(false, "Channel size is not supported for UNORM_SRGB formats! ChannelSize:", channelSize);
            break;
        }
        default:
            RS_ASSERT(false, "Type cannot be converted to DXGI_Format! Type:", (uint32)type);
            break;
        }
        return DXGI_FORMAT_UNKNOWN;
    }

    constexpr DXGI_FORMAT GetDXGIFormat(const FormatInfo& info)
    {
        return GetDXGIFormat(info.channelCount, info.bitDepth, info.formatType);
    }

    constexpr DXGI_FORMAT GetDXGIFormat(Format format)
    {
        return GetDXGIFormat(GetFormatInfo(format));
    }

    constexpr Format GetFormatFromDXGIFormat(DXGI_FORMAT format)
    {
        return GetFormat(GetFormatInfo(format));
    }
}
