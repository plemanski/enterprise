//
// Created by Peter on 5/12/2025.
//

#ifndef ROOTSIGNATURE_H
#define ROOTSIGNATURE_H
#include <cstdint>
#include <intsafe.h>
#include <wrl/client.h>

#include "directx/d3d12.h"


namespace Enterprise::Core::Graphics {
class RootSignature {
public:
    RootSignature();

    RootSignature(
        const D3D12_ROOT_SIGNATURE_DESC1 &rootSignatureDesc,
        const D3D_ROOT_SIGNATURE_VERSION  root_signature_version
    );

    virtual ~RootSignature();

    void Destroy();

    [[nodiscard]] Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRootSignature() const { return m_RootSignature; };

    [[nodiscard]] D3D12_ROOT_SIGNATURE_DESC1 GetRootSignatureDesc() const { return m_RootSignatureDesc; };

    void SetRootSignatureDesc( const D3D12_ROOT_SIGNATURE_DESC1 &rootSignatureDesc,
                               D3D_ROOT_SIGNATURE_VERSION        root_signature_version );

    uint32_t GetNumDescriptors( uint32_t rootIndex ) const;

    uint32_t GetDescriptorTableBitMask( D3D12_DESCRIPTOR_HEAP_TYPE heapType ) const;

private:
    D3D12_ROOT_SIGNATURE_DESC1                  m_RootSignatureDesc{};
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;

    uint32_t m_NumDescriptorsPerTable[32]{};
    uint32_t m_SamplerTableBitMask;
    uint32_t m_DescriptorTableBitMask;
};
}

#endif //ROOTSIGNATURE_H
