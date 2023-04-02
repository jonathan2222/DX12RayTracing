#include "PreCompiled.h"
#include "Shader.h"

#include <fstream>
#include <sstream>

bool RS::DX12::Shader::Create(const Description& description)
{
	if (!ValidateShaderTypes(description.typeFlags))
		return false;

	m_ShaderPath = RS_SHADER_PATH + description.path; // Mainly for debug purposes.

	// Path can be a path to a folder or a path with a filename (including or excluding the extension). Like this:
	// shaderPasses/coolShaderPass or shaderPasses/coolShaderPass.ps (for pixel shader) or a generic shaderPasses/coolShaderPass.hlsl
	// In this case 'coolShaderPass' migh be a folder, or it might be the name of the shader files, all of which uses the same name.
	// It might also be that the shader only has one file, but has different entry points within it for the different shader types.

	TypeFlags remainingTypesToCompile = description.typeFlags;
	TypeFlags finalTypes = TypeFlag::NONE;
	TypeFlags totalTypesSeen = TypeFlag::NONE;

	auto path = std::filesystem::path(m_ShaderPath);
	if (std::filesystem::is_directory(path)) // Folder with multiple shader files.
	{
		if (std::filesystem::is_empty(path))
		{
			LOG_ERROR("Found shader folder, but it is empty! Path: {}", m_ShaderPath.c_str());
			return false;
		}

		for (const auto& entry : std::filesystem::directory_iterator(path))
		{
			if (!CreateShaderPartsFromFile(entry.path(), remainingTypesToCompile, finalTypes, totalTypesSeen, false))
				return false;
		}

		// Validate types.
		if (remainingTypesToCompile != TypeFlag::AUTO && remainingTypesToCompile != TypeFlag::NONE)
		{
			LOG_ERROR("Could not find all types. Some are remaining to be compiled! Path: {}", m_ShaderPath);
			return false;
		}

		if ((totalTypesSeen & finalTypes) != TypeFlag::NONE)
		{
			LOG_WARNING("There are one or more shader types that are unused in the files! Path {}", m_ShaderPath);
		}

		return true;
	}
	else if (path.extension().empty() == false) // Single file.
	{
		if (std::filesystem::exists(path))
		{
			return CreateShaderPartsFromFile(path, remainingTypesToCompile, finalTypes, totalTypesSeen, true);
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
				if (!CreateShaderPartsFromFile(filePath, remainingTypesToCompile, finalTypes, totalTypesSeen, false))
					return false;
			}
		}

		// Validate types.
		if (remainingTypesToCompile != TypeFlag::AUTO && remainingTypesToCompile != TypeFlag::NONE)
		{
			LOG_ERROR("Could not find all types. Remaining types are left! Path: {}", m_ShaderPath);
			return false;
		}

		if ((totalTypesSeen & finalTypes) != TypeFlag::NONE)
		{
			LOG_WARNING("There are one or more shader types that are unused in the files! Path {}", m_ShaderPath);
		}

		return true;
	}
}

void RS::DX12::Shader::Release()
{
	for (PartData& part : m_ShaderParts)
	{
		DX12_RELEASE(part.pPDBData);
		DX12_RELEASE(part.pPDBPathFromCompiler);
		DX12_RELEASE(part.pReflection);
		DX12_RELEASE(part.pShaderObject);
		part.type = TypeFlag::NONE;
	}
}

bool RS::DX12::Shader::CreateShaderPartsFromFile(const std::filesystem::path& filePath, TypeFlags& remainingTypesToCompile, TypeFlags& finalTypes, TypeFlags& totalTypesSeen, bool isTheOnlyFile)
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
		LOG_WARNING("Duplicate types found! Choosing the first one. FilePath: {}", filePath.string());
		typesInFile = typesInFile & ~(typesInFile & finalTypes);
	}

	// Fetch types to compile.
	TypeFlags currentTypes = remainingTypesToCompile;
	if (currentTypes == TypeFlag::AUTO)
	{
		currentTypes = typesInFile;

		if (isTheOnlyFile && !ValidateShaderTypes(currentTypes))
			return false;
	}

	// Can validate the types earlier for single file shaders.
	if (isTheOnlyFile && !ValidateSingleFileTypes(currentTypes, typesInFile))
		return false;

	// Compile shaders.
	uint8 result = 3; // 2: true
	for (uint32 typeIndex = 0; typeIndex < TypeFlag::COUNT; typeIndex++)
	{
		TypeFlags typeToCheck = 1 << typeIndex;
		if ((typeToCheck & currentTypes) & typesInFile)
		{
			result &= CreateShaderPart(file, typeToCheck) ? 1 : 0;
			if (result)
			{
				if (remainingTypesToCompile != TypeFlag::AUTO)
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

RS::DX12::Shader::TypeFlags RS::DX12::Shader::GetShaderTypesFromFile(const File& file) const
{
	TypeFlags types = TypeFlag::NONE;
	if (!file.pData || file.size == 0)
		return types;

	// TODO: Make this code pretty. By using tokens and loop through them to check if they match.

	// NOTE: The order of these words need to match the order of the flags!!!!!
	char* typeStrings[TypeFlag::COUNT - 1] = { "VertexMain(", "PixelMain(", "ComputeMain(", "GeometryMain(" };
	uint32 typeStringsLen[ARRAYSIZE(typeStrings)];
	for (uint32 i = 0; i < ARRAYSIZE(typeStrings); ++i) typeStringsLen[i] = strlen(typeStrings[i]);

	uint32 j = 0;
	for (uint64 i = 0; i < file.size; ++i)
	{
		char c = ((char*)file.pData)[i];
		bool found = false;
		for (uint8 t = 0; t < TypeFlag::COUNT - 1; ++t)
		{
			uint32 len = strlen(typeStrings[t]);
			if (j < len && c == typeStrings[t][j])
			{
				found = true;
				j++;
				if (j == len) // Match!!
				{
					switch (t)
					{
					case 0: types |= TypeFlag::VERTEX; break;
					case 1: types |= TypeFlag::PIXEL; break;
					case 2: types |= TypeFlag::COMPUTE; break;
					case 3: types |= TypeFlag::GEOMETRY; break;
					default:
						LOG_WARNING("Shader Parser Failed! Type is not supported!");
						break;
					}
				}
				break;
			}
		}
		if (!found) j = 0;
	}

	return types;
}

bool RS::DX12::Shader::ValidateSingleFileTypes(TypeFlags types, TypeFlags typesInFile)
{
	TypeFlags combine = types & typesInFile;
	if (combine == 0)
	{
		LOG_ERROR("No type matches the ones in the shader file! Path: {}", m_ShaderPath);
		return false;
	}
	else if (combine != (types | typesInFile))
	{
		if ((~types) & typesInFile)
			LOG_WARNING("Not all shader types in the file are used! Path: {}", m_ShaderPath);
		else if (types & (~typesInFile))
		{
			LOG_ERROR("One or more types are not in the shader file! Path: {}", m_ShaderPath);
			return false;
		}
	}
	return true;
}

bool RS::DX12::Shader::ValidateShaderTypes(TypeFlags types) const
{
	if (types == TypeFlag::NONE)
	{
		LOG_ERROR("Invalid shader types. Zero shader types were used!");
		return false;
	}
	else if (types == TypeFlag::AUTO)
	{
		return true;
	}

	if ((types & TypeFlag::COMPUTE) && (types & ~TypeFlag::COMPUTE))
	{
		LOG_ERROR("Cannot have a compute shader together with other types!");
		return false;
	}

	return true;
}

bool RS::DX12::Shader::CheckValidExtension(const std::filesystem::path& path) const
{
	if (path.has_extension())
	{
		static const std::filesystem::path extensions[] = { L".hlsl", L".ps", L".vs", L".gs", L".cs" };
		std::filesystem::path extension = path.extension();
		for (auto& ext : extensions)
		{
			if (ext == extension)
				return true;
		}
	}
	return false;
}

bool RS::DX12::Shader::CheckIfOnlyOneTypeIsSet(TypeFlags type) const
{
	return IS_SINGLE_FLAG(type);
}

bool RS::DX12::Shader::CreateShaderPart(const File& file, TypeFlags type)
{
	if (!CheckIfOnlyOneTypeIsSet(type))
	{
		LOG_ERROR("Cannot compile a shader part with multiple types!");
		return false;
	}

	// These objects are not thread safe, create a speparate instance for each thread.
	ComPtr<IDxcUtils> utils;
	ComPtr<IDxcIncludeHandler> includeHandler;
	ComPtr<IDxcCompiler3> pCompiler;
	{ // TODO: Move this outside of here and in to a initialize function that all shaders uses. Shader library class?
		DXCallVerbose(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils.ReleaseAndGetAddressOf())));
		DXCallVerbose(utils->CreateDefaultIncludeHandler(includeHandler.ReleaseAndGetAddressOf()));
		DXCallVerbose(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler)));
	}

	std::wstring entryPoint = L"Main";
	std::wstring version = L"6_0";
	std::vector<LPCWSTR> arguments;

	// Entry point
	{
		arguments.push_back(L"-E");
		if (type & TypeFlag::VERTEX) entryPoint = L"VertexMain";
		else if (type & TypeFlag::PIXEL) entryPoint = L"PixelMain";
		else if (type & TypeFlag::COMPUTE) entryPoint = L"ComputeMain";
		else if (type & TypeFlag::GEOMETRY) entryPoint = L"GeometryMain";
		arguments.push_back(entryPoint.c_str());
	}

	{ // Target Profile
		arguments.push_back(L"-T");
		if (type & TypeFlag::VERTEX) version = L"vs_" + version;
		else if (type & TypeFlag::PIXEL) version = L"ps_" + version;
		else if (type & TypeFlag::COMPUTE) version = L"cs_" + version;
		else if (type & TypeFlag::GEOMETRY) version = L"gs_" + version;
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

	//for (const std::wstring& define : defines)
	//{
	//	arguments.push_back(L"-D");
	//	arguments.push_back(define.c_str());
	//}

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
			LOG_ERROR("Failed to compile shader part! Compile returned HRESULT: {:#10x}, Errors/Warnings:\n{}", hr, compileErrors);
			return false;
		}

		if (FAILED(hr))
		{
			LOG_ERROR("Failed to compile shader! Path: {}", m_ShaderPath);
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

RS::DX12::Shader::File RS::DX12::Shader::ReadFile(const std::string& path) const
{
	File file;
	if (std::filesystem::exists(path) == false)
	{
		LOG_ERROR("Shader file does not exist! Path: {}", path.c_str());
		return file;
	}

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
