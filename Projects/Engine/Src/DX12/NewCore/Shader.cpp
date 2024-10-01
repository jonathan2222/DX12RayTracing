#include "PreCompiled.h"
#include "Shader.h"

#include "Utils/Utils.h"
#include "Core/LaunchArguments.h"

#include <fstream>
#include <sstream>
#include <unordered_set>

namespace RS::_ShaderInternal
{
	struct ShaderTypeInfo
	{
		Shader::TypeFlags type;
		const char* name;
		const char* defaultEntryPoint;
		const char* typeStr; // e.g vs gs ps etc.
		const char* extension;
	};

#define DEF_SHADER_TYPE(type, name, defaultEntryPoint, typeStr, extension) {type, name, defaultEntryPoint, typeStr, extension},
	constexpr ShaderTypeInfo g_ShaderTypeInfos[] = {
#include "ShaderTypes.h"
	};
#undef DEF_SHADER_TYPE

#define DEF_SHADER_TYPE(type, name, defaultEntryPoint, typeStr, extension) name,
	constexpr const char* g_ShaderTypeNames[] = {
#include "ShaderTypes.h"
	};
#undef DEF_SHADER_TYPE

#define DEF_SHADER_TYPE(type, name, defaultEntryPoint, typeStr, extension) defaultEntryPoint,
	constexpr const char* g_ShaderTypeDefaultEntryPoints[] = {
#include "ShaderTypes.h"
	};
#undef DEF_SHADER_TYPE

#define DEF_SHADER_TYPE(type, name, defaultEntryPoint, typeStr, extension) typeStr,
	constexpr const char* g_ShaderTypeStrs[] = {
#include "ShaderTypes.h"
	};
#undef DEF_SHADER_TYPE

#define DEF_SHADER_TYPE(type, name, defaultEntryPoint, typeStr, extension) extension,
	constexpr const char* g_ShaderTypeExtensions[] = {
	".hlsl",
#include "ShaderTypes.h"
	};
#undef DEF_SHADER_TYPE
}

RS::Shader::~Shader()
{
	for (auto& part : m_ShaderParts)
	{
		if (part.pPDBData || part.pPDBPathFromCompiler || part.pReflection || part.pShaderObject || part.type)
			LOG_ERROR("Shader part was not released!");
	}
}

bool RS::Shader::Create(const Description& description)
{
	if (!ValidateShaderTypes(description.typeFlags))
		return false;

	m_ShaderVirtualPath = description.path;
	m_ShaderPath = RS_SHADER_PATH + m_ShaderVirtualPath; // Mainly for debug purposes.

	if (LaunchArguments::Contains(LaunchParams::logShaderDebug))
		LOG_INFO("Compiling shader with types: {}, Path: {}", TypesToString(description.typeFlags), m_ShaderPath);

	ConstructEntryPointsArray(description.customEntryPoints);

	// Path can be a path to a folder or a path with a filename (including or excluding the extension). Like this:
	// shaderPasses/coolShaderPass or shaderPasses/coolShaderPass.ps (for pixel shader) or a generic shaderPasses/coolShaderPass.hlsl
	// In this case 'coolShaderPass' migh be a folder, or it might be the name of the shader files, all of which uses the same name.
	// It might also be that the shader only has one file, but has different entry points within it for the different shader types.

	TypeFlags remainingTypesToCompile = description.typeFlags;
	TypeFlags totalTypesSeen = TypeFlag::NONE;
	m_Types = TypeFlag::NONE;

	auto path = std::filesystem::path(m_ShaderPath);
	if (std::filesystem::is_directory(path)) // Folder with multiple shader files, with different names and extensions.
	{
		if (std::filesystem::is_empty(path))
		{
			LOG_ERROR("Found shader folder, but it is empty! Path: {}", m_ShaderPath.c_str());
			return false;
		}

		for (const auto& entry : std::filesystem::directory_iterator(path))
		{
			if (!CreateShaderPartsFromFile(entry.path(), remainingTypesToCompile, m_Types, totalTypesSeen, false))
				return false;
		}

		if (!ValidateShaderTypesMatches(remainingTypesToCompile, m_Types, totalTypesSeen))
			return false;

		return true;
	}
	else if (path.extension().empty() == false) // Single file.
	{
		if (std::filesystem::exists(path))
		{
			return CreateShaderPartsFromFile(path, remainingTypesToCompile, m_Types, totalTypesSeen, true);
		}
		else
		{
			LOG_ERROR("Could not find shader file! Path: {}", m_ShaderPath.c_str());
			return false;
		}
	}
	else // Assume there are multiple shader files with the same name but different extensions.
	{
		auto stem = path.stem();
		auto pathDir = path.parent_path();
		for (const auto& entry : std::filesystem::directory_iterator(pathDir))
		{
			auto filePath = entry.path();
			auto eStem = filePath.stem();
			if (eStem == stem)
			{
				if (!CreateShaderPartsFromFile(filePath, remainingTypesToCompile, m_Types, totalTypesSeen, false))
					return false;
			}
		}

		if (!ValidateShaderTypesMatches(remainingTypesToCompile, m_Types, totalTypesSeen))
			return false;

		return true;
	}
}

void RS::Shader::Release()
{
	for (PartData& part : m_ShaderParts)
	{
		DX12_RELEASE(part.pPDBData);
		DX12_RELEASE(part.pPDBPathFromCompiler);
		DX12_RELEASE(part.pReflection);
		DX12_RELEASE(part.pShaderObject);
		part.type = TypeFlag::NONE;
	}
	m_ShaderPath = "";
}

void RS::Shader::Invalidate()
{
	for (PartData& part : m_ShaderParts)
	{
		part.pPDBData = nullptr;
		part.pPDBPathFromCompiler = nullptr;;
		part.pReflection = nullptr;
		part.pShaderObject = nullptr;
		part.type = TypeFlag::NONE;
	}
	m_ShaderPath = "";
}

bool RS::Shader::Combine(Shader& other)
{
	TypeFlags overlappingTypes = m_Types & other.m_Types;
	if (overlappingTypes)
	{
		LOG_ERROR("Failed to combine shaders! Types overlap!");
		return false;
	}

	auto it = m_ShaderParts.insert(m_ShaderParts.end(), other.m_ShaderParts.begin(), other.m_ShaderParts.end());
	if (it == m_ShaderParts.end())
		return false;

	// TODO: Might want to split this into an array?
	m_ShaderPath += other.m_ShaderPath;
	m_Types |= other.m_Types;

	other.Invalidate();

	return true;
}

D3D12_SHADER_BYTECODE RS::Shader::GetShaderByteCode(TypeFlags type, bool supressWarnings) const
{
	IDxcBlob* pShaderObject = GetShaderBlob(type, supressWarnings);
	if (pShaderObject)
		return CD3DX12_SHADER_BYTECODE(pShaderObject->GetBufferPointer(), pShaderObject->GetBufferSize());
	return CD3DX12_SHADER_BYTECODE(nullptr, 0);
}

IDxcBlob* RS::Shader::GetShaderBlob(TypeFlags type, bool supressWarnings) const
{
	if (!Utils::IsPowerOfTwo(type))
	{
		LOG_ERROR("Trying to fetch shader blob using multiple types! Type: {}", TypesToString(type));
		return nullptr;
	}

	for (const PartData& part : m_ShaderParts)
	{
		if (part.type & type)
			return part.pShaderObject;
	}

	if (!supressWarnings)
		LOG_WARNING("Fetching non-existing shader blob type! Type: {}", TypesToString(type));
	return nullptr;
}

ID3D12ShaderReflection* RS::Shader::GetReflection(TypeFlags type, bool supressWarnings) const
{
	if (!Utils::IsPowerOfTwo(type))
	{
		LOG_ERROR("Trying to fetch shader reflection using multiple types! Type: {}", TypesToString(type));
		return nullptr;
	}

	for (const PartData& part : m_ShaderParts)
	{
		if (part.type & type)
			return part.pReflection;
	}

	if (!supressWarnings)
		LOG_WARNING("Fetching non-existing shader reflection type! Type: {}", TypesToString(type));
	return nullptr;
}

bool RS::Shader::CreateShaderPartsFromFile(const std::filesystem::path& filePath, TypeFlags& remainingTypesToCompile, TypeFlags& finalTypes, TypeFlags& totalTypesSeen, bool isTheOnlyFile)
{
	// Load shader and its types.
	if (!CheckValidExtension(filePath))
	{
		LOG_ERROR("Extension is not supported as a shader file! FilePath: {}", filePath.string());
		return false;
	}

	File file = ReadFile(filePath.string());

	TypeFlags typesInFile = GetShaderTypesFromFile(file);

	totalTypesSeen |= typesInFile;

	// Check for duplicates (from different files) if AUTO was enabled.
	if (typesInFile & finalTypes)
	{
		std::string duplicateTypesStr = TypesToString(typesInFile & finalTypes);
		LOG_WARNING("Duplicate types found! Choosing the first one. Current Duplicate types: {}, FilePath: {}", duplicateTypesStr, filePath.string());
		typesInFile = typesInFile & ~(typesInFile & finalTypes);
	}

	// Fetch types to compile.
	TypeFlags currentTypes = remainingTypesToCompile;
	if (currentTypes == TypeFlag::Auto)
	{
		currentTypes = typesInFile;

		if (isTheOnlyFile && !ValidateShaderTypes(currentTypes))
			return false;
	}

	// Can validate the types earlier for single file shaders.
	if (isTheOnlyFile && !ValidateSingleFileTypes(currentTypes, typesInFile))
		return false;

	// Compile shaders.
	// TODO: Use a TriBool instead.
	uint8 result = 3; // 2: true
	for (uint32 typeIndex = 0; typeIndex < TypeFlag::COUNT; typeIndex++)
	{
		TypeFlags typeToCheck = 1 << typeIndex;
		if ((typeToCheck & currentTypes) & typesInFile)
		{
			result &= CompileShaderPart(file, typeToCheck) ? 1 : 0;
			if (result)
			{
				if (remainingTypesToCompile != TypeFlag::Auto)
					remainingTypesToCompile = remainingTypesToCompile & ~typeToCheck;
				finalTypes |= typeToCheck;
			}
		}
	}

	file.Release();

	if (result == 3 && LaunchArguments::Contains(LaunchParams::logShaderDebug))
		LOG_WARNING("No match of types in shader file! Path: {}", filePath.string());

	return result != 0;
}

RS::Shader::TypeFlags RS::Shader::GetShaderTypesFromFile(const File& file) const
{
	TypeFlags types = TypeFlag::NONE;
	if (!file.pData || file.size == 0)
		return types;

	if (LaunchArguments::Contains(LaunchParams::logShaderDebug))
		LOG_INFO("Parsing file: {}", file.name);

	// TODO: Make this code pretty. By using tokens and loop through them to check if they match. Then we can extend it to preprocess the shaders with more functionality.

	constexpr uint32 typeCount = TypeFlag::COUNT - 1;
	std::array<std::string, typeCount> entryPointStringsToSearch;
	for (uint32 i = 0; i < typeCount; i++)
		entryPointStringsToSearch[i] = m_EntryPointStrings[i].empty() ? "" : m_EntryPointStrings[i] + '(';

	uint32 cIndex = 0;
	std::unordered_set<uint8> typeIndexMatches;
	for (uint64 i = 0; i < file.size; ++i)
	{
		char c = ((char*)file.pData)[i];
		bool found = false;
		for (uint8 typeIndex = 0; typeIndex < typeCount; ++typeIndex)
		{
			uint32 len = (uint32)entryPointStringsToSearch[typeIndex].length();
			if (len > 0 && cIndex < len && c == entryPointStringsToSearch[typeIndex][cIndex])
			{
				if (cIndex != 0 && !typeIndexMatches.count(typeIndex))
					continue;

				found = true;
				cIndex++;
				typeIndexMatches.insert(typeIndex);
				if (cIndex == len) // Match!!
				{
					uint32 typeFlag = 1 << typeIndex;
					if ((TypeFlag::MASK & typeFlag) && !(typeFlag & TypeFlag::Auto))
					{
						types |= typeFlag;
						if (LaunchArguments::Contains(LaunchParams::logShaderDebug))
							LOG_INFO("\tFound type {}", TypesToString(typeFlag));
					}
					else
					{
						LOG_WARNING("Shader Parser Failed! Type is not supported!");
					}
				}
				break;
			}
		}
		if (!found)
		{
			cIndex = 0;
			typeIndexMatches.clear();
		}
	}

	return types;
}

bool RS::Shader::ValidateSingleFileTypes(TypeFlags types, TypeFlags typesInFile)
{
	TypeFlags combine = types & typesInFile;
	if (combine == 0)
	{
		std::string typeToFind = TypesToString(types);
		LOG_ERROR("No type matches the ones in the shader file! Types to find: {}, Path: {}", typeToFind, m_ShaderPath);
		return false;
	}
	else if (combine != (types | typesInFile))
	{
		if ((~types) & typesInFile)
		{
			std::string unusedTypes = TypesToString((~types) & typesInFile);
			LOG_WARNING("Not all shader types in the file are used! Unused types: {}, Path: {}", unusedTypes, m_ShaderPath);
		}
		else if (types & (~typesInFile))
		{
			std::string missingTypes = TypesToString(types & (~typesInFile));
			LOG_ERROR("One or more types are not in the shader file! Missing types: {}, Path: {}", missingTypes, m_ShaderPath);
			return false;
		}
	}
	return true;
}

bool RS::Shader::ValidateShaderTypes(TypeFlags types) const
{
	if (types == TypeFlag::NONE)
	{
		LOG_ERROR("Invalid shader types. Zero shader types were used!");
		return false;
	}
	else if (types == TypeFlag::Auto)
	{
		return true;
	}

	if ((types & TypeFlag::Compute) && (types & ~TypeFlag::Compute))
	{
		LOG_ERROR("Cannot have a compute shader together with other types!");
		return false;
	}

	return true;
}

bool RS::Shader::ValidateShaderTypesMatches(TypeFlags remainingTypesToCompile, TypeFlags finalTypes, TypeFlags totalTypesSeen) const
{
	if ((totalTypesSeen & (~finalTypes)) != TypeFlag::NONE)
	{
		std::string unusedTypes = TypesToString(totalTypesSeen & (~finalTypes));
		LOG_WARNING("There are one or more shader types that are unused in the files! Unused types: {}, Path {}", unusedTypes, m_ShaderPath);
	}

	if (remainingTypesToCompile != TypeFlag::Auto && remainingTypesToCompile != TypeFlag::NONE)
	{
		std::string remainingTypesToCompileStr = TypesToString(remainingTypesToCompile);
		LOG_ERROR("Could not find all types. Remaining types are left! Remaining types: {}, Path: {}", remainingTypesToCompileStr, m_ShaderPath);
		return false;
	}

	return true;
}

bool RS::Shader::CheckValidExtension(const std::filesystem::path& path) const
{
	if (path.has_extension())
	{
		std::filesystem::path extension = path.extension();
		for (auto& ext : _ShaderInternal::g_ShaderTypeExtensions)
		{
			if (ext == extension)
				return true;
		}
	}
	return false;
}

bool RS::Shader::CheckIfOnlyOneTypeIsSet(TypeFlags type) const
{
	return Utils::IsPowerOfTwo(type);
}

bool RS::Shader::CompileShaderPart(const File& file, TypeFlags type)
{
	std::string typeStr = TypesToString(type);
	if (!CheckIfOnlyOneTypeIsSet(type))
	{
		LOG_ERROR("Cannot compile a shader part with multiple types! Types passed: {}, Path: {}", typeStr, m_ShaderPath);
		return false;
	}

	// These objects are not thread safe, create a speparate instance for each thread.
	Microsoft::WRL::ComPtr<IDxcUtils> utils;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler;
	Microsoft::WRL::ComPtr<IDxcCompiler3> pCompiler;
	{ // TODO: Move this outside of here and in to an initialize function that all shaders uses. Shader library class?
		DXCallVerbose(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils.ReleaseAndGetAddressOf())));
		DXCallVerbose(utils->CreateDefaultIncludeHandler(includeHandler.ReleaseAndGetAddressOf()));
		DXCallVerbose(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler)));
	}

	std::wstring sourceName = file.name.empty() ? L"" : Utils::ToWString(file.name);
	std::wstring entryPoint = L"Main";
	std::wstring version = L"6_0";
	std::vector<LPCWSTR> arguments;

	uint32 typeIndex = Utils::IndexOfPow2Bit(type);

	// Entry point
	{
		arguments.push_back(L"-E");

		entryPoint = Utils::ToWString(m_EntryPointStrings[typeIndex]);
		arguments.push_back(entryPoint.c_str());
	}

	{ // Target Profile
		arguments.push_back(L"-T");

		version = Utils::ToWString(_ShaderInternal::g_ShaderTypeStrs[typeIndex]) + L"_" + version;
		arguments.push_back(version.c_str());
	}

	// Strip data from the shader blob (can still retrive it from the compile results)
	arguments.push_back(L"-Qstrip_debug");
	arguments.push_back(L"-Qstrip_reflect");
	//arguments.push_back(L"-Qstrip_priv");
	arguments.push_back(L"-Qstrip_rootsignature");

	arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS); //-WX
	arguments.push_back(DXC_ARG_DEBUG); //-Zi
	//arguments.push_back(L"-Fd");
	//arguments.push_back(L"<pdbPath>");
	//arguments.push_back(L"-Fre");
	//arguments.push_back(L"<reflectionPath>");
	arguments.push_back(DXC_ARG_PACK_MATRIX_ROW_MAJOR); //-Zp

	if (!file.name.empty())
	{
		arguments.push_back(DXC_ARG_DEBUG_NAME_FOR_SOURCE);
		arguments.push_back(sourceName.c_str());
	}

	//for (const std::wstring& define : defines)
	//{
	//	arguments.push_back(L"-D");
	//	arguments.push_back(define.c_str());
	//}

	if (LaunchArguments::Contains(LaunchParams::logShaderDebug))
		LOG_INFO("Compiling shader part {}", typeStr);

	DxcBuffer sourceBuffer;
	sourceBuffer.Ptr = file.pData;
	sourceBuffer.Size = file.size;
	sourceBuffer.Encoding = 0;

	ComPtr<IDxcResult> pCompileResult;
	DXCall(pCompiler->Compile(&sourceBuffer, arguments.data(), (uint32)arguments.size(), includeHandler.Get(), IID_PPV_ARGS(pCompileResult.GetAddressOf())));

	{
		HRESULT hr;
		pCompileResult->GetStatus(&hr);

		// Assumes default utf8 encoding, use IDxcUtf16 with -encoding utf16
		ComPtr<IDxcBlobUtf8> errorMsgs;
		DXCallVerbose(pCompileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errorMsgs), nullptr));

		if (errorMsgs && errorMsgs->GetStringLength())
		{
			const char* compileErrors = (const char*)errorMsgs->GetStringPointer();
			LOG_ERROR("Failed to compile shader part! Type: {}, Path: {}\nCompile returned HRESULT: {:#10x}, Errors/Warnings:\n{}", typeStr, file.name, hr, compileErrors);
			return false;
		}

		if (FAILED(hr))
		{
			LOG_ERROR("Failed to compile shader! Type: {}, Path: {}", typeStr, file.name);
			return false;
		}
	}

	PartData partData;
	partData.type = type;

	// Get shader object that should be passed to DX12.
	DXCallVerbose(pCompileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&partData.pShaderObject), nullptr));

	// Generate PDB data for use in PIX for debugging.
	DXCallVerbose(pCompileResult->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&partData.pPDBData), &partData.pPDBPathFromCompiler));
	// TODO: Write contents of the pdbData blob to a file for the path 'pdbPathFromCompiler'

	// Generate reflection of shader.
	{
		ComPtr<IDxcBlob> reflection;
		DXCallVerbose(pCompileResult->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflection), nullptr));

		DxcBuffer reflectionData =
		{
			reflection->GetBufferPointer(),
			reflection->GetBufferSize(),
			0U
		};

		DXCallVerbose(utils->CreateReflection(&reflectionData, IID_PPV_ARGS(&partData.pReflection)));
	}

	m_ShaderParts.push_back(partData);

	return true;
}

void RS::Shader::ConstructEntryPointsArray(const std::vector<std::pair<TypeFlags, std::string>>& customEntryPoints)
{
	constexpr uint32 typeCount = TypeFlag::COUNT - 1;

	// Fill entryPoints with default entry points and custom entry points if they exist.
	for (uint32 i = 0; i < typeCount; i++)
	{
		auto it = std::find_if(customEntryPoints.begin(), customEntryPoints.end(),
			[&](const std::pair<TypeFlags, std::string>& entry)->bool {
				return Utils::IndexOfPow2Bit(entry.first) == i;
			});

		if (it == customEntryPoints.end())
			m_EntryPointStrings[i] = _ShaderInternal::g_ShaderTypeDefaultEntryPoints[i];
		else
			m_EntryPointStrings[i] = it->second;
	}

	if (LaunchArguments::Contains(LaunchParams::logShaderDebug))
	{
		std::string entryPoints;
		for (uint32 i = 0; i < m_EntryPointStrings.size(); i++)
			entryPoints += (i == 0 ? "" : ", ") + m_EntryPointStrings[i];
		LOG_INFO("Using entry point set: {}", entryPoints);
	}
}

RS::Shader::File RS::Shader::ReadFile(const std::string& path) const
{
	File file;
	if (std::filesystem::exists(path) == false)
	{
		LOG_ERROR("Shader file does not exist! Path: {}", path.c_str());
		return file;
	}

	// For debug purposes.
	file.name = path;

	std::ifstream stream(path, std::ios::binary | std::ios::ate);
	if (stream.is_open())
	{
		file.size = stream.tellg();
		stream.seekg(0, std::ios::beg);

		file.pData = new uint8[file.size];
		stream.read((char*)file.pData, file.size);
		return file;
	}
	else
	{
		LOG_ERROR("Failed to read file!");
		return file;
	}

	return file;
}

std::string RS::Shader::TypesToString(TypeFlags types) const
{
	std::string result;

	const uint32 typeCountWihtoutAUTO = TypeFlag::COUNT - 1;
	uint32 processedCount = 0;
	for (uint32 typeIndex = 0; typeIndex < typeCountWihtoutAUTO; typeIndex++)
	{
		if ((1 << typeIndex) & types)
		{
			result += (processedCount++ == 0 ? "" : " | ") + std::string(_ShaderInternal::g_ShaderTypeNames[typeIndex]);
		}
	}

	if (types & TypeFlag::Auto)
	{
		result += (processedCount++ == 0 ? "" : " | ") + std::string("AUTO");
	}

	if (!result.empty())
	{
		std::for_each(result.begin(), result.end(), [](char& c) {
			c = std::toupper(c);
			});
	}

	return result;
}
