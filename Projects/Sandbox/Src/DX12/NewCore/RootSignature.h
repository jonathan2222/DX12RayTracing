#pragma once

#include "DX12/Dx12Device.h"

namespace RS
{
	/** Usage:
	* RootSignature rootSignature(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	* 
	* rootSignature[0]Constants(..);
	* rootSignature[0]CBV(..);
	* rootSignature[2].SetVisibility(D3D12_SHADER_VISIBILITY_PIXEL);
	* rootSignature[2][0].UAV(3, 0); // Function as the normal Init on a CD3DX12_DESCRIPTOR_RANGE1
	* rootSignature[2][1].SRVBindless(0, 0); // Sets the count to a maximum value and uses the flag D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE
	* 
	* rootSignature->Bake();
	*/

	class RootSignature
	{
	public:
		struct Entry;
		struct TableRange : public CD3DX12_DESCRIPTOR_RANGE1
		{
			static const uint32 BINDLESS_RESOURCE_MAX_SIZE = 1024;

			TableRange() { NumDescriptors = 0u; }
			void SRV(uint32 numDescriptors, uint32 baseShaderRegister, uint32 registerSpace = 0, D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE, uint32 offsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
			void UAV(uint32 numDescriptors, uint32 baseShaderRegister, uint32 registerSpace = 0, D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE, uint32 offsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
			void CBV(uint32 numDescriptors, uint32 baseShaderRegister, uint32 registerSpace = 0, D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE, uint32 offsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
			void SAMPLER(uint32 numDescriptors, uint32 baseShaderRegister, uint32 registerSpace = 0, D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE, uint32 offsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
			void SRVBindless(uint32 baseShaderRegister, uint32 registerSpace = 0, uint32 offsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
			void UAVBindless(uint32 baseShaderRegister, uint32 registerSpace = 0, uint32 offsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
			void CBVBindless(uint32 baseShaderRegister, uint32 registerSpace = 0, uint32 offsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
			void SAMPLERBindless(uint32 baseShaderRegister, uint32 registerSpace = 0, uint32 offsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
		private:
			friend struct Entry;

			bool IsNull() const { return NumDescriptors == 0; }
		};

		struct Entry
		{
			Entry() : m_IsNull(true) {}

			void SetVisibility(D3D12_SHADER_VISIBILITY visibility) { m_Visibility = visibility; }

			void SRV(uint32 shaderRegister, uint32 registerSpace = 0, D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
			void CBV(uint32 shaderRegister, uint32 registerSpace = 0, D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
			void UAV(uint32 shaderRegister, uint32 registerSpace = 0, D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
			void Constants(uint32 num32BitValues, uint32 shaderRegister, uint32 registerSpace = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

			/**
			* Create a new table range or return the existing one.
			* If there was no table range at the index or was missing one before it, then thoes will be null ranges.
			*/
			TableRange& operator[](uint32 index);

			/**
			* Returns false if there are ranges of sampler and other types together. Cannot have samplers in the same table as UAV, SRV, and CBV.
			* Also return false if there are any null ranges present.
			*/
			bool Validate() const;

			bool IsTable() const { return !m_Ranges.empty(); }

		private:
			bool IsNull() const;
			void ConstructTable();

		private:
			friend class RootSignature;

			bool m_IsNull = true;
			uint32 m_Index = UINT32_MAX;
			std::vector<TableRange> m_Ranges;
			D3D12_SHADER_VISIBILITY m_Visibility = D3D12_SHADER_VISIBILITY_ALL;
			CD3DX12_ROOT_PARAMETER1 m_RootParameter;
		};
	public:
		explicit RootSignature(D3D12_ROOT_SIGNATURE_FLAGS flags, D3D_ROOT_SIGNATURE_VERSION requestedVersion = D3D_ROOT_SIGNATURE_VERSION_1_1);

		void SRV(uint32 shaderRegister, uint32 registerSpace = 0, D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
		void CBV(uint32 shaderRegister, uint32 registerSpace = 0, D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
		void UAV(uint32 shaderRegister, uint32 registerSpace = 0, D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
		void Constants(uint32 num32BitValues, uint32 shaderRegister, uint32 registerSpace = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

		/**
		* Create a new entry or return the existing one.
		* If there was no entry at the index or was missing one before it, then thoes will be null entries.
		*/
		Entry& operator[](uint32 index);

		Entry& AddEntry();

		void AddStaticSampler(CD3DX12_STATIC_SAMPLER_DESC sampler);

		/**
		* Used to create the final root signature.
		*/
		void Bake();

		uint32 GetDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE heapType) const;

		Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRootSignature() const { return m_RootSignature; }
		const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& GetRootSignatureDesc() const;

		uint32 GetNumDescriptors(uint32 rootIndex) const;

	private:
		bool Validate() const;

		void Free();
		void CreateRootSignature();
		void ConstructDescriptorTableBitMasks();

	private:
		std::vector<Entry> m_Entries;
		std::vector<CD3DX12_STATIC_SAMPLER_DESC> m_StaticSamplers;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;

		bool m_Dirty = false;
		uint32	m_DescriptorTableBitMasks[2]; // Only CBV_SRV_UAV and SAMPLER are allowed.
		D3D12_ROOT_SIGNATURE_FLAGS m_Flags;
		D3D_ROOT_SIGNATURE_VERSION m_Version;
		D3D12_VERSIONED_ROOT_SIGNATURE_DESC m_VersionedRootSignatureDesc;
	};
}
