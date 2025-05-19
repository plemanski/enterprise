//
// Created by Peter on 5/12/2025.
//

#include "Resource.h"

#include <utility>
#include "Helpers.h"
#include "Device.h"
#include "DirectXTex.h"
#include "Renderer.h"
#include "ResourceStateTracker.h"
#include "directx/d3dx12.h"

namespace Enterprise::Core::Graphics {
Resource::Resource(const std::wstring& name)
    : m_ResourceName(name)
    , m_FormatSupport({})
{}

Resource::Resource(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue, const std::wstring& name)
{
    if ( clearValue )
    {
        m_D3D12ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*clearValue);
    }

    auto device = Renderer::Get()->GetDevice();

    ThrowIfFailed( device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON,
        m_D3D12ClearValue.get(),
        IID_PPV_ARGS(&m_D3D12Resource)
    ) );

    ResourceStateTracker::AddGlobalResourceState(m_D3D12Resource.Get(), D3D12_RESOURCE_STATE_COMMON );

    CheckFeatureSupport();
    SetName(name);
}

Resource::Resource(Microsoft::WRL::ComPtr<ID3D12Resource> resource, const std::wstring& name)
    : m_D3D12Resource(resource)
    , m_FormatSupport({})
{
    CheckFeatureSupport();
    SetName(name);
}

Resource::Resource(const Resource& copy)
    : m_D3D12Resource(copy.m_D3D12Resource)
    , m_FormatSupport(copy.m_FormatSupport)
    , m_ResourceName(copy.m_ResourceName)
    , m_D3D12ClearValue( copy.m_D3D12ClearValue ? std::make_unique<D3D12_CLEAR_VALUE>(*copy.m_D3D12ClearValue) : nullptr )
{}

Resource::Resource(Resource&& copy)
    : m_D3D12Resource(std::move(copy.m_D3D12Resource))
    , m_FormatSupport(copy.m_FormatSupport)
    , m_ResourceName(std::move(copy.m_ResourceName))
    , m_D3D12ClearValue(std::move(copy.m_D3D12ClearValue))
{}

Resource& Resource::operator=(const Resource& other)
{
    if ( this != &other )
    {
        m_D3D12Resource = other.m_D3D12Resource;
        m_FormatSupport = other.m_FormatSupport;
        m_ResourceName = other.m_ResourceName;
        if ( other.m_D3D12ClearValue )
        {
            m_D3D12ClearValue = std::make_unique<D3D12_CLEAR_VALUE>( *other.m_D3D12ClearValue );
        }
    }

    return *this;
}

Resource& Resource::operator=(Resource&& other) noexcept
{
    if (this != &other)
    {
        m_D3D12Resource = std::move(other.m_D3D12Resource);
        m_FormatSupport = other.m_FormatSupport;
        m_ResourceName = std::move(other.m_ResourceName);
        m_D3D12ClearValue = std::move( other.m_D3D12ClearValue );

        other.Reset();
    }

    return *this;
}


Resource::~Resource()
{
}

void Resource::SetD3D12Resource( Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource,
                                 const D3D12_CLEAR_VALUE*               clearValue )
{
    m_D3D12Resource = std::move(d3d12Resource);
    if (m_D3D12ClearValue)
    {
        m_D3D12ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*clearValue);
    } else
    {
        m_D3D12ClearValue.reset();
    }
    CheckFeatureSupport();
    SetName(m_ResourceName);
}

void Resource::SetName( const std::wstring &name )
{
    m_ResourceName = name;
    if (m_D3D12Resource && !m_ResourceName.empty())
    {
        m_D3D12Resource->SetName(m_ResourceName.c_str());
    }
}

void Resource::Reset()
{
    m_D3D12Resource.Reset();
    m_FormatSupport = {};
    m_D3D12ClearValue.reset();
    m_ResourceName.clear();
}

bool Resource::CheckFormatSupport( D3D12_FORMAT_SUPPORT1 formatSupport ) const
{
    return (m_FormatSupport.Support1 & formatSupport) != 0;
}

bool Resource::CheckFormatSupport( D3D12_FORMAT_SUPPORT2 formatSupport ) const
{
    return (m_FormatSupport.Support2 & formatSupport) != 0;
}

void Resource::CheckFeatureSupport()
{
    auto d3d12Device = Renderer::Get()->GetDevice();

    auto desc = m_D3D12Resource->GetDesc();
    m_FormatSupport.Format = desc.Format;
    ThrowIfFailed(d3d12Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &m_FormatSupport,
                                                   sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT)));
}

Texture::Texture( const std::wstring &name )
    : Resource(name)
{
    CreateViews();
}

Texture::Texture( const D3D12_RESOURCE_DESC &resourceDesc, const D3D12_CLEAR_VALUE* clearValue,
    const std::wstring &name )
        : Resource(resourceDesc, clearValue, name)
{
    CreateViews();
}

Texture::Texture( Microsoft::WRL::ComPtr<ID3D12Resource> resource, const std::wstring &name )
    : Resource(resource, name)
{
    CreateViews();
}

Texture::Texture(const Texture& copy)
    : Resource(copy)
{
    CreateViews();
}

Texture::Texture(Texture&& copy)
    : Resource(copy)
{
    CreateViews();
}

Texture& Texture::operator=(const Texture& other)
{
    Resource::operator=(other);

    CreateViews();

    return *this;
}
Texture& Texture::operator=(Texture&& other)
{
    Resource::operator=(other);

    CreateViews();

    return *this;
}

Texture::~Texture()
{}


void Texture::Resize( uint32_t width, uint32_t height, uint32_t depth )
{
    if (m_D3D12Resource)
    {
        CD3DX12_RESOURCE_DESC resourceDesc(m_D3D12Resource->GetDesc());

        resourceDesc.Width = std::max(width, 1u);
        resourceDesc.Height = std::max(height, 1u);
        resourceDesc.DepthOrArraySize = depth;
        resourceDesc.MipLevels = resourceDesc.SampleDesc.Count > 1 ? 1 : 0;

        auto d3d12Device = Renderer::Get()->GetDevice();

        ThrowIfFailed(d3d12Device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                D3D12_HEAP_FLAG_NONE, &resourceDesc,
                D3D12_RESOURCE_STATE_COMMON,
                m_D3D12ClearValue.get(),
                IID_PPV_ARGS(&m_D3D12Resource))
        );

        m_D3D12Resource->SetName(m_ResourceName.c_str());

        ResourceStateTracker::AddGlobalResourceState(m_D3D12Resource.Get(), D3D12_RESOURCE_STATE_COMMON);

        CreateViews();
    }
}

D3D12_UNORDERED_ACCESS_VIEW_DESC GetUAVDesc( const D3D12_RESOURCE_DESC &resDesc, UINT mipSlice, UINT arraySlice = 0,
                                             UINT                       planeSlice = 0 )
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = resDesc.Format;

    switch (resDesc.Dimension)
    {
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            if (resDesc.DepthOrArraySize > 1)
            {
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
                uavDesc.Texture1DArray.ArraySize = resDesc.DepthOrArraySize - arraySlice;
                uavDesc.Texture1DArray.FirstArraySlice = arraySlice;
                uavDesc.Texture1DArray.MipSlice = mipSlice;
            } else
            {
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
                uavDesc.Texture1D.MipSlice = mipSlice;
            }
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            if (resDesc.DepthOrArraySize > 1)
            {
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                uavDesc.Texture2DArray.ArraySize = resDesc.DepthOrArraySize - arraySlice;
                uavDesc.Texture2DArray.FirstArraySlice = arraySlice;
                uavDesc.Texture2DArray.PlaneSlice = planeSlice;
                uavDesc.Texture2DArray.MipSlice = mipSlice;
            } else
            {
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                uavDesc.Texture2D.PlaneSlice = planeSlice;
                uavDesc.Texture2D.MipSlice = mipSlice;
            }
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
            uavDesc.Texture3D.WSize = resDesc.DepthOrArraySize - arraySlice;
            uavDesc.Texture3D.FirstWSlice = arraySlice;
            uavDesc.Texture3D.MipSlice = mipSlice;
            break;
        default:
            throw std::exception("Invalid resource dimension.");
    }

    return uavDesc;
}

void Texture::CreateViews()
{
    if (m_D3D12Resource)
    {
        auto d3d12Device = Renderer::Get()->GetDevice();
        auto renderer = Renderer::Get();

        CD3DX12_RESOURCE_DESC desc(m_D3D12Resource->GetDesc());

        // Create RTV
        if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0 && CheckRTVSupport())
        {
            m_RenderTargetView = renderer->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            d3d12Device->CreateRenderTargetView(m_D3D12Resource.Get(), nullptr,
                                                m_RenderTargetView.GetDescriptorHandle());
        }
        // Create DSV
        if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0 && CheckDSVSupport())
        {
            m_DepthStencilView = renderer->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            d3d12Device->CreateDepthStencilView(m_D3D12Resource.Get(), nullptr,
                                                m_DepthStencilView.GetDescriptorHandle());
        }
        // Create SRV
        if ((desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0 && CheckSRVSupport())
        {
            m_ShaderResourceView = renderer->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            d3d12Device->CreateShaderResourceView(m_D3D12Resource.Get(), nullptr,
                                                  m_ShaderResourceView.GetDescriptorHandle());
        }
        // Create UAV for each mip (only supported for 1D and 2D textures).
        if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0 && CheckUAVSupport() &&
            desc.DepthOrArraySize == 1)
        {
            m_UnorderedAccessView =
                    renderer->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, desc.MipLevels);
            for (int i = 0; i < desc.MipLevels; ++i)
            {
                auto uavDesc = GetUAVDesc(desc, i);
                d3d12Device->CreateUnorderedAccessView(m_D3D12Resource.Get(), nullptr, &uavDesc,
                                                       m_UnorderedAccessView.GetDescriptorHandle(i));
            }
        }
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetRenderTargetView() const
{
    return m_RenderTargetView.GetDescriptorHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetDepthStencilView() const
{
    return m_DepthStencilView.GetDescriptorHandle();
}


DescriptorAllocation Texture::CreateShaderResourceView( const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc ) const
{
    auto renderer = Renderer::Get();
    auto device = renderer->GetDevice();
    auto srv = renderer->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    device->CreateShaderResourceView(m_D3D12Resource.Get(), srvDesc, srv.GetDescriptorHandle());

    return srv;
}

DescriptorAllocation Texture::CreateUnorderedAccessView( const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc ) const
{
    auto renderer = Renderer::Get();
    auto device = renderer->GetDevice();
    auto uav = renderer->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    device->CreateUnorderedAccessView(m_D3D12Resource.Get(), nullptr, uavDesc, uav.GetDescriptorHandle());

    return uav;
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetShaderResourceView( const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc ) const
{
    std::size_t hash = 0;
    if (srvDesc)
    {
        //This might not work
        hash = std::hash<const D3D12_SHADER_RESOURCE_VIEW_DESC *>{}(srvDesc);
    }

    std::lock_guard<std::mutex> lock(m_ShaderResourceViewsMutex);

    auto iter = m_ShaderResourceViews.find(hash);
    if (iter == m_ShaderResourceViews.end())
    {
        auto srv = CreateShaderResourceView(srvDesc);
        iter = m_ShaderResourceViews.insert({hash, std::move(srv)}).first;
    }

    return iter->second.GetDescriptorHandle();
}

//D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetShaderResourceView() const
//{
//    return m_ShaderResourceView.GetDescriptorHandle();
//}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetUnorderedAccessView( uint32_t mip ) const
{
    return m_UnorderedAccessView.GetDescriptorHandle(mip);
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc) const
{
    std::size_t hash = 0;
    if (uavDesc)
    {
        hash = std::hash<D3D12_UNORDERED_ACCESS_VIEW_DESC>{}(*uavDesc);
    }

    std::lock_guard<std::mutex> guard(m_UnorderedAccessViewsMutex);

    auto iter = m_UnorderedAccessViews.find(hash);
    if (iter == m_UnorderedAccessViews.end())
    {
        auto uav = CreateUnorderedAccessView(uavDesc);
        iter = m_UnorderedAccessViews.insert( { hash, std::move(uav) }).first;
    }

    return iter->second.GetDescriptorHandle();
}

bool Texture::HasAlpha() const
{
    DXGI_FORMAT format = GetD3D12ResourceDesc().Format;

    bool hasAlpha = false;

    switch (format)
    {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_B5G5R5A1_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
        case DXGI_FORMAT_A8P8:
        case DXGI_FORMAT_B4G4R4A4_UNORM:
            hasAlpha = true;
            break;
    }

    return hasAlpha;
}

size_t Texture::BitsPerPixel() const
{
    auto format = GetD3D12ResourceDesc().Format;
    return DirectX::BitsPerPixel(format);
}

bool Texture::IsUAVCompatibleFormat( DXGI_FORMAT format )
{
    switch (format)
    {
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SINT:
            return true;
        default:
            return false;
    }
}

bool Texture::IsSRGBFormat( DXGI_FORMAT format )
{
    switch (format)
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return true;
        default:
            return false;
    }
}

bool Texture::IsBGRFormat( DXGI_FORMAT format )
{
    switch (format)
    {
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            return true;
        default:
            return false;
    }
}

bool Texture::IsDepthFormat( DXGI_FORMAT format )
{
    switch (format)
    {
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_D16_UNORM:
            return true;
        default:
            return false;
    }
}

DXGI_FORMAT Texture::GetTypelessFormat( DXGI_FORMAT format )
{
    DXGI_FORMAT typelessFormat = format;

    switch (format)
    {
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
            typelessFormat = DXGI_FORMAT_R32G32B32A32_TYPELESS;
            break;
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
            typelessFormat = DXGI_FORMAT_R32G32B32_TYPELESS;
            break;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
            typelessFormat = DXGI_FORMAT_R16G16B16A16_TYPELESS;
            break;
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
            typelessFormat = DXGI_FORMAT_R32G32_TYPELESS;
            break;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            typelessFormat = DXGI_FORMAT_R32G8X24_TYPELESS;
            break;
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
            typelessFormat = DXGI_FORMAT_R10G10B10A2_TYPELESS;
            break;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
            typelessFormat = DXGI_FORMAT_R8G8B8A8_TYPELESS;
            break;
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_SINT:
            typelessFormat = DXGI_FORMAT_R16G16_TYPELESS;
            break;
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
            typelessFormat = DXGI_FORMAT_R32_TYPELESS;
            break;
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_SINT:
            typelessFormat = DXGI_FORMAT_R8G8_TYPELESS;
            break;
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_SINT:
            typelessFormat = DXGI_FORMAT_R16_TYPELESS;
            break;
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
            typelessFormat = DXGI_FORMAT_R8_TYPELESS;
            break;
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
            typelessFormat = DXGI_FORMAT_BC1_TYPELESS;
            break;
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
            typelessFormat = DXGI_FORMAT_BC2_TYPELESS;
            break;
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
            typelessFormat = DXGI_FORMAT_BC3_TYPELESS;
            break;
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
            typelessFormat = DXGI_FORMAT_BC4_TYPELESS;
            break;
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
            typelessFormat = DXGI_FORMAT_BC5_TYPELESS;
            break;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            typelessFormat = DXGI_FORMAT_B8G8R8A8_TYPELESS;
            break;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            typelessFormat = DXGI_FORMAT_B8G8R8X8_TYPELESS;
            break;
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
            typelessFormat = DXGI_FORMAT_BC6H_TYPELESS;
            break;
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            typelessFormat = DXGI_FORMAT_BC7_TYPELESS;
            break;
    }

    return typelessFormat;
}

DXGI_FORMAT Texture::GetSRGBFormat( DXGI_FORMAT format )
{
    DXGI_FORMAT srgbFormat = format;
    switch (format)
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            srgbFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            break;
        case DXGI_FORMAT_BC1_UNORM:
            srgbFormat = DXGI_FORMAT_BC1_UNORM_SRGB;
            break;
        case DXGI_FORMAT_BC2_UNORM:
            srgbFormat = DXGI_FORMAT_BC2_UNORM_SRGB;
            break;
        case DXGI_FORMAT_BC3_UNORM:
            srgbFormat = DXGI_FORMAT_BC3_UNORM_SRGB;
            break;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            srgbFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
            break;
        case DXGI_FORMAT_B8G8R8X8_UNORM:
            srgbFormat = DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
            break;
        case DXGI_FORMAT_BC7_UNORM:
            srgbFormat = DXGI_FORMAT_BC7_UNORM_SRGB;
            break;
    }

    return srgbFormat;
}

DXGI_FORMAT Texture::GetUAVCompatableFormat( DXGI_FORMAT format )
{
    DXGI_FORMAT uavFormat = format;

    switch (format)
    {
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            uavFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
            break;
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
            uavFormat = DXGI_FORMAT_R32_FLOAT;
            break;
    }

    return uavFormat;
}
}
