#pragma once

#include "DX12/NewCore/DX12Core3.h"

#include <dxcapi.h>
#include <filesystem>

namespace RS
{
	class Shader
	{
	public:
		RS_BEGIN_BITFLAGS_U32(TypeFlag)
			RS_BITFLAG(Vertex)		// Vertex shader
			RS_BITFLAG(Pixel)		// Pixel shader
			RS_BITFLAG(Compute)	// Compute shader
			RS_BITFLAG(Geometry)	// Geometry shader
			RS_BITFLAG(Domain)		// Domain shader
			RS_BITFLAG(Hull)		// Hull shader
			RS_BITFLAG(Auto)		// Let the shader find all the shader types.
		RS_END_BITFLAGS();

		struct Description
		{
			std::string path;
			TypeFlags	typeFlags = TypeFlag::Auto;
			// If typeFlags is AUTO, it will load all shader types it sees and put them into the same shader.
			// If it is something else, it will only load the specific types, even though there might be more in the source!
			// TODO: Make it able to combine different Shader objects into one, if the user choose to load one shader part per shader instance?
			std::vector<std::pair<TypeFlags, std::string>> customEntryPoints;
		};

	public:
		~Shader();

		bool Create(const Description& description);
		void Release();
		void Invalidate();

		/*
		* Transfers and combines onership from one shader to this, validates the types. Return true if succeeded, otherwise false.
		*/
		bool Combine(Shader& other);

		D3D12_SHADER_BYTECODE GetShaderByteCode(TypeFlags type, bool supressWarnings = false) const;
		IDxcBlob* GetShaderBlob(TypeFlags type, bool supressWarnings = false) const;
		ID3D12ShaderReflection* GetReflection(TypeFlags type, bool supressWarnings = false) const;

	private:
		struct File
		{
			std::string name;
			uint8* pData = nullptr;
			uint64 size = 0;

			void Release()
			{
				if (pData) delete[] pData;
				pData = nullptr;
				size = 0;
				name.clear();
			}
		};

		bool CreateShaderPartsFromFile(const std::filesystem::path& filePath, TypeFlags& remainingTypesToCompile, TypeFlags& finalTypes, TypeFlags& totalTypesSeen, bool isTheOnlyFile);

		/* 
		* Parse file to find the entry points. It will use the default entry points if not a custom one was specified.
		* It will ignore a type if a custom entry point was specified with an empty string.
		*/
		TypeFlags GetShaderTypesFromFile(const File& file) const;
		bool ValidateSingleFileTypes(TypeFlags types, TypeFlags typesInFile);
		bool ValidateShaderTypes(TypeFlags types) const;
		bool ValidateShaderTypesMatches(TypeFlags remainingTypesToCompile, TypeFlags finalTypes, TypeFlags totalTypesSeen) const;
		bool CheckValidExtension(const std::filesystem::path& path) const;
		bool CheckIfOnlyOneTypeIsSet(TypeFlags types) const;
		bool CompileShaderPart(const File& file, TypeFlags type);

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
}