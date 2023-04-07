#pragma once

#include "DX12/Dx12Core2.h"

#include <dxcapi.h>
#include <filesystem>

namespace RS::DX12
{
	class Shader
	{
	public:
		BEGIN_BITFLAGS_U32(TypeFlag)
			BITFLAG(VERTEX)		// Vertex shader
			BITFLAG(PIXEL)		// Pixel shader
			BITFLAG(COMPUTE)	// Compute shader
			BITFLAG(GEOMETRY)	// Geometry shader
			BITFLAG(AUTO)		// Let the shader find all the shader types.
		END_BITFLAGS();

		struct Description
		{
			std::string path;
			TypeFlags	typeFlags = TypeFlag::AUTO;
			// If typeFlags is AUTO, it will load all shader types it sees and put them into the same shader.
			// If it is something else, it will only load the specific types, even though there might be more in the source!
			// TODO: Make it able to combine different Shader objects into one, if the user choose to load one shader part per shader instance?
		};

	public:
		bool Create(const Description& description);
		void Release();

		/* Combine one shader with this, validates the types. Return true if succeeded, otherwise false.
		*/
		bool Combine(const Shader& other);

	private:
		struct File
		{
			uint8* pData = nullptr;
			uint64 size = 0;

			void Release()
			{
				if (pData) delete[] pData;
				pData = nullptr;
				size = 0;
			}
		};

		bool CreateShaderPartsFromFile(const std::filesystem::path& filePath, TypeFlags& remainingTypesToCompile, TypeFlags& finalTypes, TypeFlags& totalTypesSeen, bool isTheOnlyFile);

		/* 
		* Parse file to find the entry points. It will use the default entry points if not a custom one was specified.
		* It will ignore a type if a custom entry point was specified with an empty string.
		*/
		TypeFlags GetShaderTypesFromFile(const File& file, const std::vector<std::pair<TypeFlags, std::string>>& customEntryPoints) const;
		bool ValidateSingleFileTypes(TypeFlags types, TypeFlags typesInFile);
		bool ValidateShaderTypes(TypeFlags types) const;
		bool ValidateShaderTypesMatches(TypeFlags remainingTypesToCompile, TypeFlags finalTypes, TypeFlags totalTypesSeen) const;
		bool CheckValidExtension(const std::filesystem::path& path) const;
		bool CheckIfOnlyOneTypeIsSet(TypeFlags types) const;
		bool CreateShaderPart(const File& file, TypeFlags type);

		void ConstructEntryPointsArray(const std::vector<std::pair<TypeFlags, std::string>>& customEntryPoints);

		File ReadFile(const std::string& path) const;

		std::string TypesToString(TypeFlags types) const;

	private:
		struct PartData
		{
			TypeFlags				type = TypeFlag::NONE;
			IDxcBlob*				pShaderObject = nullptr;
			ID3D12ShaderReflection* pReflection = nullptr;
			IDxcBlob*				pPDBData = nullptr;
			IDxcBlobUtf16*			pPDBPathFromCompiler = nullptr;
		};

		TypeFlags				m_Types = TypeFlag::NONE;
		std::string				m_ShaderPath;
		std::vector<PartData>	m_ShaderParts;

		// Temp data
		std::array<std::string, TypeFlag::COUNT - 1> m_EntryPointStrings;
	};

	// TODO: Might want to move this to the cpp file.
	namespace _ShaderInternal
	{
		struct ShaderTypeInfo
		{
			Shader::TypeFlags type;
			char* name;
			char* defaultEntryPoint;
			char* typeStr; // e.g vs gs ps etc.
			char* extension;
		};

#define DEF_SHADER_TYPE(type, name, defaultEntryPoint, typeStr, extension) {type, name, defaultEntryPoint, typeStr, extension},
		constexpr ShaderTypeInfo g_ShaderTypeInfos[] = {
#include "ShaderTypes.h"
		};
#undef DEF_SHADER_TYPE

#define DEF_SHADER_TYPE(type, name, defaultEntryPoint, typeStr, extension) name,
		constexpr char* g_ShaderTypeNames[] = {
#include "ShaderTypes.h"
		};
#undef DEF_SHADER_TYPE

#define DEF_SHADER_TYPE(type, name, defaultEntryPoint, typeStr, extension) defaultEntryPoint,
		constexpr char* g_ShaderTypeDefaultEntryPoints[] = {
#include "ShaderTypes.h"
		};
#undef DEF_SHADER_TYPE

#define DEF_SHADER_TYPE(type, name, defaultEntryPoint, typeStr, extension) typeStr,
		constexpr char* g_ShaderTypeStrs[] = {
#include "ShaderTypes.h"
		};
#undef DEF_SHADER_TYPE

#define DEF_SHADER_TYPE(type, name, defaultEntryPoint, typeStr, extension) extension,
		constexpr char* g_ShaderTypeExtensions[] = {
		".hlsl",
#include "ShaderTypes.h"
		};
#undef DEF_SHADER_TYPE
	}
}