//
// Created by Peter on 5/12/2025.
//

#ifndef RESOURCE_H
#define RESOURCE_H
#define NOMINMAX
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <wrl/client.h>

#include "DescriptorAllocation.h"
#include "Core.h"

#include "directx/d3d12.h"


namespace Enterprise::Core::Graphics {
class Device;

class ENTERPRISE_API Resource {
public:
    explicit Resource(const std::wstring& name = L"");
    explicit Resource(const D3D12_RESOURCE_DESC& resourceDesc,
        const D3D12_CLEAR_VALUE* clearValue = nullptr,
        const std::wstring& name = L"");
    explicit Resource(Microsoft::WRL::ComPtr<ID3D12Resource> resource, const std::wstring& name = L"");

    Resource(const Resource& copy);
    Resource(Resource&& copy);

    Resource& operator=( const Resource& other);
    Resource& operator=(Resource&& other) noexcept;

    virtual ~Resource();
    bool IsValid() const
    {
        return ( m_D3D12Resource != nullptr );
    }

    virtual void SetD3D12Resource(Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource,
        const D3D12_CLEAR_VALUE* clearValue = nullptr );

    [[nodiscard]] Microsoft::WRL::ComPtr<ID3D12Resource> GetD3D12Resource() const { return m_D3D12Resource; }

    [[nodiscard]] D3D12_RESOURCE_DESC GetD3D12ResourceDesc() const
    {
        D3D12_RESOURCE_DESC resDesc = {};
        if (m_D3D12Resource)
        {
            resDesc = m_D3D12Resource->GetDesc();
        }
        return resDesc;
    };

    void Resource::SetName( const std::wstring &name );
    virtual void Reset();
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView( const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc ) const =
    0;

    /**
     * Get the UAV for a (sub)resource.
     *
     * @param uavDesc The description of the UAV to return.
     */
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView( const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr ) const = 0;
    [[nodiscard]] bool CheckFormatSupport( D3D12_FORMAT_SUPPORT1 format ) const;

    [[nodiscard]] bool CheckFormatSupport( D3D12_FORMAT_SUPPORT2 format ) const;

protected:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_D3D12Resource;
    D3D12_FEATURE_DATA_FORMAT_SUPPORT      m_FormatSupport;
    std::unique_ptr<D3D12_CLEAR_VALUE>     m_D3D12ClearValue;
    std::wstring                           m_ResourceName;

private:
    void CheckFeatureSupport();
};

class ENTERPRISE_API Texture : public Resource {
public:

    int32_t m_Width;
    int32_t m_Height;
    int32_t m_ComponentsPerPixel;
    explicit Texture( const std::wstring &name = L"" );

    explicit Texture( const D3D12_RESOURCE_DESC &resourceDesc,
                      const D3D12_CLEAR_VALUE*   clearValue = nullptr,
                      const std::wstring &       name = L"" );

    explicit Texture( Microsoft::WRL::ComPtr<ID3D12Resource> resource,
                      const std::wstring &                   name = L"" );

    Texture( const Texture &copy );

    Texture( Texture &&copy );

    Texture &operator=( const Texture &other );

    Texture &operator=( Texture &&other );

    virtual ~Texture();

    void Resize( uint32_t width, uint32_t height, uint32_t depth = 1 );

    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const;

    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;


    D3D12_CPU_DESCRIPTOR_HANDLE
    GetShaderResourceView( const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc ) const override;

    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView( uint32_t mip ) const;

    D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView( const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc ) const;

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
    DescriptorAllocation CreateShaderResourceView( const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc ) const;

    DescriptorAllocation CreateUnorderedAccessView( const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc ) const;

private:
    DescriptorAllocation m_RenderTargetView;
    DescriptorAllocation m_DepthStencilView;
    DescriptorAllocation m_ShaderResourceView;
    DescriptorAllocation m_UnorderedAccessView;

    mutable std::unordered_map<size_t, DescriptorAllocation> m_ShaderResourceViews;
    mutable std::unordered_map<size_t, DescriptorAllocation> m_UnorderedAccessViews;
    mutable std::mutex                                       m_ShaderResourceViewsMutex;
    mutable std::mutex                                       m_UnorderedAccessViewsMutex;

};
}

#endif //RESOURCE_H
