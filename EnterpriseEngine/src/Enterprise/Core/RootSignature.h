//
// Created by Peter on 5/12/2025.
//

#ifndef ROOTSIGNATURE_H
#define ROOTSIGNATURE_H
#include <cstdint>
#include <intsafe.h>
#include <wrl/client.h>

#include "directx/d3d12.h"


class RootSignature {
public:
    RootSignature();
    RootSignature(
        D3D12_ROOT_SIGNATURE_DESC1& rootSignatureDesc,
        D3D_ROOT_SIGNATURE_VERSION root_signature_version
    );

    virtual ~RootSignature();
    void Destroy();
    [[nodiscard]] Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRootSignature() const { return m_RootSignature; };

    [[nodiscard]] D3D12_ROOT_SIGNATURE_DESC1 GetRootSignatureDesc() const { return m_RootSignatureDesc; };
    void SetRootSignatureDesc( D3D12_ROOT_SIGNATURE_DESC1, D3D_ROOT_SIGNATURE_VERSION root_signature_version );

    uint32_t GetNumDescriptors( DWORD rootIndex );

    uint32_t GetDescriptorTableBitMask( D3D12_DESCRIPTOR_HEAP_TYPE heapType );

private:
    D3D12_ROOT_SIGNATURE_DESC1                      m_RootSignatureDesc{};
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_RootSignature;

    uint32_t                                        m_NumDescriptorsPerTable[32] {};
    uint32_t                                        m_SampleTableBitMask;
    uint32_t                                        m_DescriptorTableBitMask;
};



#endif //ROOTSIGNATURE_H
