//
// Created by Peter on 5/13/2025.
//

#ifndef GENERATEMIPSPSO_H
#define GENERATEMIPSPSO_H

#include "RootSignature.h"
#include "DescriptorAllocation.h"

#include "directx/d3d12.h"
#include <DirectXMath.h>
#include <wrl.h>


namespace Enterprise::Core::Graphics {
namespace GenerateMips {
    enum {
        GenerateMipsCB,
        SrcMip,
        OutMip,
        NumRootParameters
    };
}

struct alignas( 16 ) GenerateMipsCB {
    uint32_t          SrcMipsLevel; // Texture level of source mip
    uint32_t          NumMipsLevels; // Number of out mips to write 1-4
    uint32_t          SrcDimension; // Width and height of the src texture
    uint32_t          IsSRGB; // Apply gamma correction for srgb?
    DirectX::XMFLOAT2 TexelSize; // 1.0 / OutMip1.Dimensions
};

class GenerateMipsPso {
public:
    GenerateMipsPso();

    [[nodiscard]] const RootSignature &GetRootSignature() const { return m_RootSignature; }

    [[nodiscard]] Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPipeLineState() const { return m_PipelineStatue; }

    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetDefaultUAV() const { return m_DefaultUAV.GetDescriptorHandle(); }

private:
    RootSignature                               m_RootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineStatue;
    DescriptorAllocation                        m_DefaultUAV;
};
}

#endif //GENERATEMIPSPSO_H
