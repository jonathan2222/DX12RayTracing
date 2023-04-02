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

		TypeFlags GetShaderTypesFromFile(const File& file) const;
		bool ValidateSingleFileTypes(TypeFlags types, TypeFlags typesInFile);
		bool ValidateShaderTypes(TypeFlags types) const;
		bool CheckValidExtension(const std::filesystem::path& path) const;
		bool CheckIfOnlyOneTypeIsSet(TypeFlags types) const;
		bool CreateShaderPart(const File& file, TypeFlags type);

		File ReadFile(const std::string& path) const;

	private:
		struct PartData
		{
			TypeFlags				type = TypeFlag::NONE;
			IDxcBlob*				pShaderObject = nullptr;
			ID3D12ShaderReflection* pReflection = nullptr;
			IDxcBlob*				pPDBData = nullptr;
			IDxcBlobUtf16*			pPDBPathFromCompiler = nullptr;
		};

		std::string				m_ShaderPath;
		std::vector<PartData>	m_ShaderParts;
	};
}