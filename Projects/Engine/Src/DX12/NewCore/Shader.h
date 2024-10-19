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

		/*
		* If succeeds, it will create the shader. If not, it will keep the state as it were before calling Create!
		*/
		bool Create(const Description& description);
		void Release();
		void Invalidate();

		/*
		* Transfers and combines onership from one shader to this, validates the types. Return true if succeeded, otherwise false.
		*/
		bool Combine(Shader& other);

		D3D12_SHADER_BYTECODE GetShaderByteCode(TypeFlags type, bool supressWarnings = false);
		IDxcBlob* GetShaderBlob(TypeFlags type, bool supressWarnings = false);
		ID3D12ShaderReflection* GetReflection(TypeFlags type, bool supressWarnings = false);

		const std::string& GetPath() const { return m_ShaderPath; }
		const std::string& GetVirtualPath() const { return m_ShaderVirtualPath; }

		const Description& GetDescription() const { return m_Description; }

	private:
		struct PartData
		{
			TypeFlags				type = TypeFlag::NONE;
			IDxcBlob*				pShaderObject = nullptr;
			ID3D12ShaderReflection* pReflection = nullptr;
			IDxcBlob*				pPDBData = nullptr;
			IDxcBlobUtf16*			pPDBPathFromCompiler = nullptr;
		};

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

		using EntryPointsStringArray = std::array<std::string, TypeFlag::COUNT - 1>;

		bool CreateShaderPartsFromFile(const std::filesystem::path& filePath, TypeFlags& remainingTypesToCompile,
			TypeFlags& finalTypes, TypeFlags& totalTypesSeen, bool isTheOnlyFile,
			const EntryPointsStringArray& entryPointStrings, const std::string& shaderPath,
			std::vector<PartData>& shaderPartsOut);

		/* 
		* Parse file to find the entry points. It will use the default entry points if not a custom one was specified.
		* It will ignore a type if a custom entry point was specified with an empty string.
		*/
		static TypeFlags GetShaderTypesFromFile(const File& file, const EntryPointsStringArray& entryPointStrings);
		static bool ValidateSingleFileTypes(TypeFlags types, TypeFlags typesInFile, const std::string& shaderPath);
		static bool ValidateShaderTypes(TypeFlags types);
		static bool ValidateShaderTypesMatches(TypeFlags remainingTypesToCompile, TypeFlags finalTypes, TypeFlags totalTypesSeen, const std::string& shaderPath);
		static bool CheckValidExtension(const std::filesystem::path& path);
		static bool CheckIfOnlyOneTypeIsSet(TypeFlags types);
		static std::optional<PartData> CompileShaderPart(const File& file, TypeFlags type, const EntryPointsStringArray& entryPointStrings, const std::string& shaderPath);

		static EntryPointsStringArray ConstructEntryPointsArray(const std::vector<std::pair<TypeFlags, std::string>>& customEntryPoints);

		static File ReadFile(const std::string& path);

		static std::string TypesToString(TypeFlags types);

	private:
		Description				m_Description;

		std::mutex				m_Mutex;

		TypeFlags				m_Types = TypeFlag::NONE;
		std::string				m_ShaderPath;
		std::string				m_ShaderVirtualPath;
		std::vector<PartData>	m_ShaderParts;

		// Temp data
		EntryPointsStringArray m_EntryPointStrings;
	};
}