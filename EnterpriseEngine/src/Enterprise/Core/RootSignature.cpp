//
// Created by Peter on 5/12/2025.
//

#include "RootSignature.h"

#include "Renderer.h"

//TODO: Implement
namespace Enterprise::Core::Graphics {
RootSignature::RootSignature()
    : m_RootSignatureDesc{}
      , m_NumDescriptorsPerTable{0}
      , m_SamplerTableBitMask(0)
      , m_DescriptorTableBitMask(0)
{
}

RootSignature::RootSignature( const D3D12_ROOT_SIGNATURE_DESC1 &rootSignatureDesc,
                              const D3D_ROOT_SIGNATURE_VERSION  root_signature_version )
    : m_RootSignatureDesc{}
      , m_NumDescriptorsPerTable{0}
      , m_SamplerTableBitMask(0)
      , m_DescriptorTableBitMask(0)
{
};

RootSignature::~RootSignature()
{
    Destroy();
}

void RootSignature::Destroy()
{
    for (UINT i = 0; i < m_RootSignatureDesc.NumParameters; ++i)
    {
        const D3D12_ROOT_PARAMETER1 &rootParam = m_RootSignatureDesc.pParameters[i];
        if (rootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
        {
            delete[] rootParam.DescriptorTable.pDescriptorRanges;
        }
    }
    delete[] m_RootSignatureDesc.pParameters;
    m_RootSignatureDesc.pParameters = nullptr;
    m_RootSignatureDesc.NumParameters = 0;

    delete[] m_RootSignatureDesc.pStaticSamplers;
    m_RootSignatureDesc.pParameters = nullptr;
    m_RootSignatureDesc.NumParameters = 0;

    m_DescriptorTableBitMask = 0;
    m_SamplerTableBitMask = 0;

    memset(m_NumDescriptorsPerTable, 0, sizeof(m_NumDescriptorsPerTable));
}

void RootSignature::SetRootSignatureDesc( const D3D12_ROOT_SIGNATURE_DESC1 &rootSignatureDesc,
                                          D3D_ROOT_SIGNATURE_VERSION        rootSignatureVersion )
{
    Destroy();

    auto device = Renderer::Get()->GetDevice();

    UINT                   numParameters = rootSignatureDesc.NumParameters;
    D3D12_ROOT_PARAMETER1* pParameters = numParameters > 0 ? new D3D12_ROOT_PARAMETER1[numParameters] : nullptr;

    for (UINT i = 0; i < numParameters; ++i)
    {
        const D3D12_ROOT_PARAMETER1 &rootParam = rootSignatureDesc.pParameters[i];
        pParameters[i] = rootParam;

        if (rootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
        {
            UINT                     numDescriptorRanges = rootParam.DescriptorTable.NumDescriptorRanges;
            D3D12_DESCRIPTOR_RANGE1* pDescriptorRanges = numDescriptorRanges > 0
                                                             ? new D3D12_DESCRIPTOR_RANGE1[numDescriptorRanges]
                                                             : nullptr;

            memcpy(pDescriptorRanges, rootParam.DescriptorTable.pDescriptorRanges,
                   sizeof(D3D12_DESCRIPTOR_RANGE1) * numDescriptorRanges);

            pParameters[i].DescriptorTable.NumDescriptorRanges = numDescriptorRanges;
            pParameters[i].DescriptorTable.pDescriptorRanges = pDescriptorRanges;

            if (numDescriptorRanges > 0)
            {
                switch (pDescriptorRanges[0].RangeType)
                {
                    case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
                    case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
                    case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
                        m_DescriptorTableBitMask |= (1 << i);
                        break;
                    case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
                        m_SamplerTableBitMask |= (1 << i);
                        break;
                }
            }
            for (UINT j = 0; j < numDescriptorRanges; ++j)
            {
                m_NumDescriptorsPerTable[i] += pDescriptorRanges[j].NumDescriptors;
            }
        }
    }

    m_RootSignatureDesc.NumParameters = numParameters;
    m_RootSignatureDesc.pParameters = pParameters;

    UINT                       numStaticSamplers = rootSignatureDesc.NumStaticSamplers;
    D3D12_STATIC_SAMPLER_DESC* pStaticSamplers = numStaticSamplers > 0
                                                     ? new D3D12_STATIC_SAMPLER_DESC[numStaticSamplers]
                                                     : nullptr;

    if (pStaticSamplers)
    {
        memcpy(pStaticSamplers, rootSignatureDesc.pStaticSamplers,
               sizeof(D3D12_STATIC_SAMPLER_DESC) * numStaticSamplers);
    }

    m_RootSignatureDesc.NumStaticSamplers = numStaticSamplers;
    m_RootSignatureDesc.pStaticSamplers = pStaticSamplers;

    D3D12_ROOT_SIGNATURE_FLAGS flags = rootSignatureDesc.Flags;
    m_RootSignatureDesc.Flags = flags;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC versionRootSignatureDesc;
    versionRootSignatureDesc.Init_1_1(numParameters, pParameters, numStaticSamplers, pStaticSamplers, flags);

    Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&versionRootSignatureDesc,
                                                        rootSignatureVersion,
                                                        &rootSignatureBlob, &errorBlob));
}

uint32_t RootSignature::GetDescriptorTableBitMask( D3D12_DESCRIPTOR_HEAP_TYPE heapType ) const
{
    uint32_t descriptorTableBitMask = 0;
    switch (heapType)
    {
        case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
            descriptorTableBitMask = m_DescriptorTableBitMask;
            break;
        case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
            descriptorTableBitMask = m_SamplerTableBitMask;
            break;
    }

    return descriptorTableBitMask;
}


uint32_t RootSignature::GetNumDescriptors( uint32_t rootIndex ) const
{
    assert( rootIndex < 32 );
    return m_NumDescriptorsPerTable[rootIndex];
}
}
