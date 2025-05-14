//
// Created by Peter on 5/12/2025.
//

#ifndef RESOURCE_H
#define RESOURCE_H
#define NOMINMAX
#include <cstdint>
#include <string>
#include <wrl/client.h>

#include "DescriptorAllocation.h"

#include "directx/d3d12.h"


namespace Enterprise::Core::Graphics {
class Device;

class Resource {
public:
    Resource( Device &device, const D3D12_RESOURCE_DESC &resourceDesc, const D3D12_CLEAR_VALUE* clearValue );

    Resource( Device &device, Microsoft::WRL::ComPtr<ID3D12Resource> resource, const D3D12_CLEAR_VALUE* clearValue = nullptr );

    void SetD3D12Resource( Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource, const D3D12_CLEAR_VALUE* clearValue );

    ~Resource() = default;

    [[nodiscard]] Microsoft::WRL::ComPtr<ID3D12Resource> GetD3D12Resource() const { return m_D3D12Resource; }
    [[nodiscard]] D3D12_RESOURCE_DESC GetD3D12ResourceDesc() const
    {
        D3D12_RESOURCE_DESC resDesc = {};
        if ( m_D3D12Resource )
        {
            resDesc = m_D3D12Resource->GetDesc();
        }
        return resDesc;
    };
    void Resource::SetName( const std::wstring &name );

    D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView( const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc );

    [[nodiscard]] bool CheckFormatSupport( D3D12_FORMAT_SUPPORT1 format ) const;

    [[nodiscard]] bool CheckFormatSupport( D3D12_FORMAT_SUPPORT2 format ) const;

protected:
    Device &m_Device;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_D3D12Resource;
    D3D12_FEATURE_DATA_FORMAT_SUPPORT      m_FormatSupport;
    std::unique_ptr<D3D12_CLEAR_VALUE>     m_D3D12ClearValue;
    std::wstring                           m_ResourceName;

private:
    void CheckFeatureSupport();
};

class Texture : public Resource {
public:
    void Resize( uint32_t width, uint32_t height, uint32_t depth );

    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const;

    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;

    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView() const;

    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView( uint32_t mip ) const;

    [[nodiscard]] bool CheckSRVSupport() const { return CheckFormatSupport(D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE); }
    [[nodiscard]] bool CheckRTVSupport() const { return CheckFormatSupport(D3D12_FORMAT_SUPPORT1_RENDER_TARGET); }
    [[nodiscard]] bool CheckDSVSupport() const { return CheckFormatSupport(D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL); }

    [[nodiscard]] bool CheckUAVSupport() const
    {
        return CheckFormatSupport(D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) &&
               CheckFormatSupport(D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) &&
               CheckFormatSupport(D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE);
    }

    [[nodiscard]] bool HasAlpha() const;

    [[nodiscard]] size_t BitsPerPixel() const;

    static bool IsUAVCompatibleFormat( DXGI_FORMAT format );

    static bool IsSRGBFormat( DXGI_FORMAT format );

    static bool IsBGRFormat( DXGI_FORMAT format );

    static bool IsDepthFormat( DXGI_FORMAT format );

    static DXGI_FORMAT GetTypelessFormat( DXGI_FORMAT format );

    static DXGI_FORMAT GetSRGBFormat( DXGI_FORMAT format );

    static DXGI_FORMAT GetUAVCompatableFormat( DXGI_FORMAT format );

    void CreateViews();
private:
    Texture( Device &device, const D3D12_RESOURCE_DESC &resourceDesc, const D3D12_CLEAR_VALUE* clearValue = nullptr );

    Texture( Device &                 device, Microsoft::WRL::ComPtr<ID3D12Resource> resource,
             const D3D12_CLEAR_VALUE* clearValue = nullptr );

    virtual ~Texture();


private:
    DescriptorAllocation m_RenderTargetView;
    DescriptorAllocation m_DepthStencilView;
    DescriptorAllocation m_ShaderResourceView;
    DescriptorAllocation m_UnorderedAccessView;
};
}

#endif //RESOURCE_H
