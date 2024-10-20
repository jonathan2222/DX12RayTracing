#include "PreCompiled.h"
#include "RootSignature.h"

#include "DX12/NewCore/DX12Core3.h"

RS::RootSignature::RootSignature(D3D12_ROOT_SIGNATURE_FLAGS flags, D3D_ROOT_SIGNATURE_VERSION requestedVersion)
	: m_Dirty(true)
	, m_Flags(flags)
	, m_Version(requestedVersion) // TODO: Check if it supports this version!
	, m_RootSignature(nullptr)
{
	memset(m_DescriptorTableBitMasks, 0, sizeof(uint32) * 2);
}

void RS::RootSignature::SRV(uint32 shaderRegister, uint32 registerSpace, D3D12_ROOT_DESCRIPTOR_FLAGS flags, D3D12_SHADER_VISIBILITY visibility)
{
	AddEntry().SRV(shaderRegister, registerSpace, flags, visibility);
}

void RS::RootSignature::CBV(uint32 shaderRegister, uint32 registerSpace, D3D12_ROOT_DESCRIPTOR_FLAGS flags, D3D12_SHADER_VISIBILITY visibility)
{
	AddEntry().CBV(shaderRegister, registerSpace, flags, visibility);
}

void RS::RootSignature::UAV(uint32 shaderRegister, uint32 registerSpace, D3D12_ROOT_DESCRIPTOR_FLAGS flags, D3D12_SHADER_VISIBILITY visibility)
{
	AddEntry().UAV(shaderRegister, registerSpace, flags, visibility);
}

void RS::RootSignature::Constants(uint32 num32BitValues, uint32 shaderRegister, uint32 registerSpace, D3D12_SHADER_VISIBILITY visibility)
{
	AddEntry().Constants(shaderRegister, registerSpace, visibility);
}

RS::RootSignature::Entry& RS::RootSignature::operator[](uint32 index)
{
	m_Dirty = true;

	if (m_Entries.size() <= index)
		m_Entries.resize(index + 1);

	return m_Entries[index];
}

RS::RootSignature::Entry& RS::RootSignature::AddEntry()
{
	m_Dirty = true;
	m_Entries.resize(m_Entries.size() + 1);
	return m_Entries.back();
}

void RS::RootSignature::AddStaticSampler(CD3DX12_STATIC_SAMPLER_DESC sampler)
{
	m_Dirty = true;
	m_StaticSamplers.push_back(sampler);
}

void RS::RootSignature::Bake(const std::string& debugName)
{
	// TODO: Should not free a root signature if it is in use! Defer it!
	if (m_RootSignature)
		Free();

	RS_ASSERT(Validate(), "The new root signature is incomplete!");
	if (m_Dirty)
	{
		ConstructDescriptorTableBitMasks();
		CreateRootSignature(debugName);
		m_Dirty = false;
	}
	else
	{
		LOG_WARNING("Trying to bake when it is not needed!");
	}
}

uint32 RS::RootSignature::GetDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE heapType) const
{
	RS_ASSERT(!m_Dirty, "Trying to get the descriptor table bit mask when root signature needs baking!");
    RS_ASSERT_NO_MSG(heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ||
		heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    return m_DescriptorTableBitMasks[heapType];
}

const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& RS::RootSignature::GetRootSignatureDesc() const
{
	RS_ASSERT(!m_Dirty, "Trying to get the root signature desc when root signature needs baking!");
	RS_ASSERT_NO_MSG(m_RootSignature);
	return m_VersionedRootSignatureDesc;
}

uint32 RS::RootSignature::GetNumDescriptors(uint32 rootIndex) const
{
	RS_ASSERT(!m_Dirty, "Trying to get the number of descriptors when root signature needs baking!");
	RS_ASSERT_NO_MSG(rootIndex < m_VersionedRootSignatureDesc.Desc_1_1.NumParameters);

	const Entry& entry = m_Entries[rootIndex];
	if (entry.IsTable() == false)
		return 1;

	uint32 descriptorCount = 0;
	for (auto& range : entry.m_Ranges)
		descriptorCount += range.NumDescriptors;
	return descriptorCount;
}

void RS::RootSignature::UpdateHash(xxh::hash3_state_t<64>& hashStream)
{
	for (Entry& entry : m_Entries)
	{
		hashStream.update(&entry.m_RootParameter.ParameterType, sizeof(D3D12_ROOT_PARAMETER_TYPE));
		hashStream.update(&entry.m_RootParameter.ShaderVisibility, sizeof(D3D12_SHADER_VISIBILITY));
		if (entry.m_RootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
			hashStream.update(entry.m_RootParameter.DescriptorTable.pDescriptorRanges, entry.m_RootParameter.DescriptorTable.NumDescriptorRanges * sizeof(D3D12_DESCRIPTOR_RANGE1));
		else if (entry.m_RootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
			hashStream.update(&entry.m_RootParameter.Constants, sizeof(D3D12_ROOT_CONSTANTS));
		else // CBV, SRV or UAV
			hashStream.update(&entry.m_RootParameter.Descriptor, sizeof(D3D12_ROOT_DESCRIPTOR1));
	}

	hashStream.update(&m_Version, sizeof(m_Version));
	hashStream.update(&m_Flags, sizeof(m_Flags));
	hashStream.update(m_StaticSamplers);
}

bool RS::RootSignature::Validate() const
{
	if (m_Entries.empty()) return false;
	for (auto& entry : m_Entries)
	{
		if (!entry.Validate())
			return false;
	}

	return true;
}

void RS::RootSignature::Free()
{
	m_RootSignature->Release();
	m_RootSignature.Reset();
}

void RS::RootSignature::CreateRootSignature(const std::string& debugName)
{
	std::vector<CD3DX12_ROOT_PARAMETER1> rootParameters;

	for (Entry& entry : m_Entries)
	{
		if (entry.IsTable())
			entry.ConstructTable();
		rootParameters.push_back(entry.m_RootParameter);
	}

	m_VersionedRootSignatureDesc.Version = m_Version;
	m_VersionedRootSignatureDesc.Desc_1_1.Flags = m_Flags;
	m_VersionedRootSignatureDesc.Desc_1_1.NumParameters = rootParameters.size();
	m_VersionedRootSignatureDesc.Desc_1_1.pParameters = rootParameters.data();
	m_VersionedRootSignatureDesc.Desc_1_1.NumStaticSamplers = m_StaticSamplers.size();
	m_VersionedRootSignatureDesc.Desc_1_1.pStaticSamplers = m_StaticSamplers.data();

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	HRESULT hr = D3D12SerializeVersionedRootSignature(&m_VersionedRootSignatureDesc, &signature, &error);

	if (FAILED(hr))
	{
		if (error)
		{
			std::string str((char*)error->GetBufferPointer());
			LOG_ERROR("{}", str);
		}

		DXCall(hr);
	}

	auto pDevice = DX12Core3::Get()->GetD3D12Device();

	DXCall(pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));

	// Set debug name
	DXCallVerbose(m_RootSignature->SetName(Utils::ToWString(debugName).c_str()));
	m_DebugName = debugName;
}

void RS::RootSignature::ConstructDescriptorTableBitMasks()
{
	m_DescriptorTableBitMasks[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = 0;
	m_DescriptorTableBitMasks[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] = 0;
	for (uint32 i = 0; i < m_Entries.size(); ++i)
	{
		Entry& entry = m_Entries[i];
		if (entry.IsTable())
		{
			bool isSampler = entry.m_Ranges.front().RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
			if (isSampler)
				m_DescriptorTableBitMasks[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] |= 1 << i;
			else
				m_DescriptorTableBitMasks[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] |= 1 << i;
		}
	}
}

void RS::RootSignature::TableRange::SRV(uint32 numDescriptors, uint32 baseShaderRegister, uint32 registerSpace, D3D12_DESCRIPTOR_RANGE_FLAGS flags, uint32 offsetInDescriptorsFromTableStart)
{
	Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, numDescriptors, baseShaderRegister, registerSpace, flags, offsetInDescriptorsFromTableStart);
}

void RS::RootSignature::TableRange::UAV(uint32 numDescriptors, uint32 baseShaderRegister, uint32 registerSpace, D3D12_DESCRIPTOR_RANGE_FLAGS flags, uint32 offsetInDescriptorsFromTableStart)
{
	Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, numDescriptors, baseShaderRegister, registerSpace, flags, offsetInDescriptorsFromTableStart);
}

void RS::RootSignature::TableRange::CBV(uint32 numDescriptors, uint32 baseShaderRegister, uint32 registerSpace, D3D12_DESCRIPTOR_RANGE_FLAGS flags, uint32 offsetInDescriptorsFromTableStart)
{
	Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, numDescriptors, baseShaderRegister, registerSpace, flags, offsetInDescriptorsFromTableStart);
}

void RS::RootSignature::TableRange::SAMPLER(uint32 numDescriptors, uint32 baseShaderRegister, uint32 registerSpace, D3D12_DESCRIPTOR_RANGE_FLAGS flags, uint32 offsetInDescriptorsFromTableStart)
{
	Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, numDescriptors, baseShaderRegister, registerSpace, flags, offsetInDescriptorsFromTableStart);
}

void RS::RootSignature::TableRange::SRVBindless(uint32 baseShaderRegister, uint32 registerSpace, uint32 offsetInDescriptorsFromTableStart)
{
	Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, BINDLESS_RESOURCE_MAX_SIZE, baseShaderRegister, registerSpace, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, offsetInDescriptorsFromTableStart);
}

void RS::RootSignature::TableRange::UAVBindless(uint32 baseShaderRegister, uint32 registerSpace, uint32 offsetInDescriptorsFromTableStart)
{
	Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, BINDLESS_RESOURCE_MAX_SIZE, baseShaderRegister, registerSpace, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, offsetInDescriptorsFromTableStart);
}

void RS::RootSignature::TableRange::CBVBindless(uint32 baseShaderRegister, uint32 registerSpace, uint32 offsetInDescriptorsFromTableStart)
{
	Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, BINDLESS_RESOURCE_MAX_SIZE, baseShaderRegister, registerSpace, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, offsetInDescriptorsFromTableStart);
}

void RS::RootSignature::TableRange::SAMPLERBindless(uint32 baseShaderRegister, uint32 registerSpace, uint32 offsetInDescriptorsFromTableStart)
{
	Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, BINDLESS_RESOURCE_MAX_SIZE, baseShaderRegister, registerSpace, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, offsetInDescriptorsFromTableStart);
}

void RS::RootSignature::Entry::SRV(uint32 shaderRegister, uint32 registerSpace, D3D12_ROOT_DESCRIPTOR_FLAGS flags, D3D12_SHADER_VISIBILITY visibility)
{
	m_RootParameter.InitAsShaderResourceView(shaderRegister, registerSpace, flags, visibility);
	m_IsNull = false;
	m_Visibility = visibility;
}

void RS::RootSignature::Entry::CBV(uint32 shaderRegister, uint32 registerSpace, D3D12_ROOT_DESCRIPTOR_FLAGS flags, D3D12_SHADER_VISIBILITY visibility)
{
	m_RootParameter.InitAsConstantBufferView(shaderRegister, registerSpace, flags, visibility);
	m_IsNull = false;
	m_Visibility = visibility;
}

void RS::RootSignature::Entry::UAV(uint32 shaderRegister, uint32 registerSpace, D3D12_ROOT_DESCRIPTOR_FLAGS flags, D3D12_SHADER_VISIBILITY visibility)
{
	m_RootParameter.InitAsUnorderedAccessView(shaderRegister, registerSpace, flags, visibility);
	m_IsNull = false;
	m_Visibility = visibility;
}

void RS::RootSignature::Entry::Constants(uint32 num32BitValues, uint32 shaderRegister, uint32 registerSpace, D3D12_SHADER_VISIBILITY visibility)
{
	m_RootParameter.InitAsConstants(num32BitValues, shaderRegister, registerSpace, visibility);
	m_IsNull = false;
	m_Visibility = visibility;
}

RS::RootSignature::TableRange& RS::RootSignature::Entry::operator[](uint32 index)
{
	m_IsNull = false;

	if (m_Ranges.size() <= index)
		m_Ranges.resize(index+1);

	return m_Ranges[index];
}

bool RS::RootSignature::Entry::Validate() const
{
	if (IsTable())
	{
		const bool isSampler = m_Ranges.front().RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		for (auto& range : m_Ranges)
		{
			// Cannot have samplers together with the other types.
			if (isSampler != (range.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER))
				return false;

			// Cannot have a null range.
			if (range.IsNull())
				return false;
		}
	}
	else
	{
		if (IsNull())
			return false;
	}
	return true;
}

bool RS::RootSignature::Entry::IsNull() const
{
	return m_IsNull;
}

void RS::RootSignature::Entry::ConstructTable()
{
	RS_ASSERT(IsTable(), "Trying to construct a table without one!");
	m_RootParameter.InitAsDescriptorTable(m_Ranges.size(), m_Ranges.data(), m_Visibility);
}
