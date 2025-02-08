#pragma once

#include "DX12/Final/DXCore.h"

#include <dxcapi.h>
#include <filesystem>

namespace RS::DX12
{
	class DXShader
	{
	public:
		RS_ENUM_FLAGS(uint32, TypeFlag,
			Vertex,		// Vertex shader
			Pixel,		// Pixel shader
			Compute,	// Compute shader
			Geometry,	// Geometry shader
			Domain,		// Domain shader
			Hull,		// Hull shader
			Auto		// Let the shader find all the shader types.
		);

		struct Description
		{
			bool isInternalPath = false;
			std::string path;
			TypeFlag	typeFlags = TypeFlag::Auto;
			// If typeFlags is AUTO, it will load all shader types it sees and put them into the same shader.
			// If it is something else, it will only load the specific types, even though there might be more in the source!
			// TODO: Make it able to combine different Shader objects into one, if the user choose to load one shader part per shader instance?
			std::vector<std::pair<TypeFlags, std::string>> customEntryPoints;
			std::vector<std::string> defines; // Example: { "USE_BINDLESS", "COUNT 2", ... }
		};

	public:
		~DXShader();

		static std::string GetDiskPathFromVirtualPath(const std::string& virtualPath, bool isInternal);

		/*
		* If succeeds, it will create the shader. If not, it will keep the state as it were before calling Create!
		*/
		bool Create(const Description& description);
		void Release();
		void Invalidate();

		/*
		* Transfers and combines onership from one shader to this, validates the types. Return true if succeeded, otherwise false.
		*/
		bool Combine(DXShader& other);

		D3D12_SHADER_BYTECODE GetShaderByteCode(TypeFlags type, bool supressWarnings = false);
		IDxcBlob* GetShaderBlob(TypeFlags type, bool supressWarnings = false);
		ID3D12ShaderReflection* GetReflection(TypeFlags type, bool supressWarnings = false);

		const std::string& GetPath() const { return m_ShaderPath; }
		const std::string& GetVirtualPath() const { return m_ShaderVirtualPath; }

		const Description& GetDescription() const { return m_Description; }

		TypeFlag GetTypes() const { return m_Types; }

	private:
		struct PartData
		{
			TypeFlags				type = TypeFlag::NONE;
			IDxcBlob* pShaderObject = nullptr;
			ID3D12ShaderReflection* pReflection = nullptr;
			IDxcBlob* pPDBData = nullptr;
			IDxcBlobUtf16* pPDBPathFromCompiler = nullptr;
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
			std::vector<PartData>& shaderPartsOut, const std::vector<std::string>& defines);

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

		static File ReadFile(const std::string& path, const std::vector<std::string>& defines);

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