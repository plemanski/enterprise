//
// Created by Peter on 5/12/2025.
//

#include <filesystem>

#include "CommandList.h"


#include "DirectXTex.h"
#include "IndexBuffer.h"
#include "Log.h"
#include "Resource.h"
#include "ResourceStateTracker.h"


namespace Enterprise::Core::Graphics {
std::map<std::wstring, ID3D12Resource *> CommandList::ms_TextureCache{};
std::mutex                               CommandList::ms_TextureCacheMutex{};

CommandList::CommandList( D3D12_COMMAND_LIST_TYPE type )
    : m_D3D12CommandListType( type )
{
    auto device = Renderer::Get()->GetDevice();

    ThrowIfFailed( device->CreateCommandAllocator( m_D3D12CommandListType, IID_PPV_ARGS( &m_D3D12CommandAllocator ) ) );

    ThrowIfFailed( device->CreateCommandList( 0, m_D3D12CommandListType, m_D3D12CommandAllocator.Get(),
                                              nullptr, IID_PPV_ARGS( &m_D3D12CommandList ) ) );

    m_UploadBuffer = std::make_unique<UploadBuffer>();

    m_ResourceStateTracker = std::make_unique<ResourceStateTracker>();

    for ( int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i )
    {
        m_DynamicDescriptorHeap[i] = std::make_unique<DynamicDescriptorHeap>( static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>( i ) );
        m_DynamicDescriptors[i] = nullptr;
    }
}

void CommandList::SetGraphicsDynamicConstantBuffer( uint32_t    rootParameterIndex, size_t sizeInBytes,
                                                    const void* bufferData )
{
    auto alloc = m_UploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    memcpy(alloc.CPU, bufferData, sizeInBytes);

    m_D3D12CommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, alloc.GPU);
}

void CommandList::SetShaderResourceView( uint32_t                               rootParameterIndex,
                                         uint32_t                               descriptorOffset,
                                         const Resource &                       resource,
                                         D3D12_RESOURCE_STATES                  stateAfter,
                                         UINT                                   firstSubResource,
                                         UINT                                   numSubResources,
                                         const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc )
{
    if (numSubResources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
    {
        for (uint32_t i = 0; i < numSubResources; ++i)
        {
            TransitionBarrier(resource, stateAfter, firstSubResource + i);
        }
    } else
    {
        TransitionBarrier(resource, stateAfter);
    }

    m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(
        rootParameterIndex, descriptorOffset, 1, resource.GetShaderResourceView(srvDesc));

    TrackResource(resource);
}

void CommandList::SetRenderTarget(const RenderTarget& renderTarget )
{
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> renderTargetDescriptors;
    renderTargetDescriptors.reserve(AttachmentPoint::NumAttachmentPoints);

    const auto& textures = renderTarget.GetTextures();

    // Bind color targets (max of 8 render targets can be bound to the rendering pipeline.
    for ( int i = 0; i < 8; ++i )
    {
        auto& texture = textures[i];

        if ( texture.IsValid() )
        {
            TransitionBarrier( texture, D3D12_RESOURCE_STATE_RENDER_TARGET );
            renderTargetDescriptors.push_back( texture.GetRenderTargetView() );

            TrackResource( texture );
        }
    }

    const auto& depthTexture = renderTarget.GetTexture( AttachmentPoint::DepthStencil );

    CD3DX12_CPU_DESCRIPTOR_HANDLE depthStencilDescriptor(D3D12_DEFAULT);
    if (depthTexture.GetD3D12Resource())
    {
        TransitionBarrier(depthTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        depthStencilDescriptor = depthTexture.GetDepthStencilView();

        TrackResource(depthTexture);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE* pDSV = depthStencilDescriptor.ptr != 0 ? &depthStencilDescriptor : nullptr;

    m_D3D12CommandList->OMSetRenderTargets( static_cast<UINT>( renderTargetDescriptors.size() ),
        renderTargetDescriptors.data(), FALSE, pDSV );
}

void CommandList::Draw( uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance )
{
    FlushResourceBarriers();

    for (const auto &heap: m_DynamicDescriptorHeap)
    {
        heap->CommitStagedDescriptorsForDraw(*this);
    }

    m_D3D12CommandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
}

void CommandList::DrawIndexed( uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndex,
                               uint32_t startVertex, uint32_t           startInstance )
{
    FlushResourceBarriers();

    for (const auto &heap: m_DynamicDescriptorHeap)
    {
        heap->CommitStagedDescriptorsForDraw(*this);
    }
    m_D3D12CommandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndex, startInstance,
                                             startVertex);
}

void CommandList::CopyResource(Microsoft::WRL::ComPtr<ID3D12Resource> dstRes, Microsoft::WRL::ComPtr<ID3D12Resource> srcRes)
{
    TransitionBarrier(dstRes, D3D12_RESOURCE_STATE_COPY_DEST);
    TransitionBarrier(srcRes, D3D12_RESOURCE_STATE_COPY_SOURCE);

    FlushResourceBarriers();

    m_D3D12CommandList->CopyResource(dstRes.Get(), srcRes.Get());

    TrackResource(dstRes);
    TrackResource(srcRes);
}

void CommandList::CopyResource( Resource &destResource, const Resource &srcResource )
{
    this->CopyResource(destResource.GetD3D12Resource().Get(), srcResource.GetD3D12Resource().Get());
}

void CommandList::CopyTextureSubresource( const Texture &texture, uint32_t firstSubResource, uint32_t numSubResources,
                                          D3D12_SUBRESOURCE_DATA* data )
{
    auto device = Renderer::Get()->GetDevice();
    auto destResource = texture.GetD3D12Resource();
    if (destResource)
    {
        TransitionBarrier(texture, D3D12_RESOURCE_STATE_COPY_DEST);
        FlushResourceBarriers();
        uint64_t requiredSize = GetRequiredIntermediateSize(destResource.Get(), firstSubResource, numSubResources);
        Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;
        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(requiredSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&intermediateResource)
        ));

        UpdateSubresources( m_D3D12CommandList.Get(), destResource.Get(), intermediateResource.Get(),
            0, firstSubResource, numSubResources, data);

        TrackObject(intermediateResource);
        TrackObject(destResource);
    }
}

void CommandList::CopyBuffer( Buffer& buffer, size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags )
{
    auto device = Renderer::Get()->GetDevice();

    size_t bufferSize = numElements * elementSize;

    Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource;
    if ( bufferSize == 0 )
    {
        // This will result in a NULL resource (which may be desired to define a default null resource).
    }
    else
    {
        ThrowIfFailed( device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT ),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags),
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&d3d12Resource)));

        // Add the resource to the global resource state tracker.
        ResourceStateTracker::AddGlobalResourceState( d3d12Resource.Get(), D3D12_RESOURCE_STATE_COMMON);

        if ( bufferData != nullptr )
        {
            // Create an upload resource to use as an intermediate buffer to copy the buffer resource
            Microsoft::WRL::ComPtr<ID3D12Resource> uploadResource;
            ThrowIfFailed( device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&uploadResource)));

            D3D12_SUBRESOURCE_DATA subresourceData = {};
            subresourceData.pData = bufferData;
            subresourceData.RowPitch = bufferSize;
            subresourceData.SlicePitch = subresourceData.RowPitch;

            m_ResourceStateTracker->TransitionResource(d3d12Resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST);
            FlushResourceBarriers();

            UpdateSubresources( m_D3D12CommandList.Get(), d3d12Resource.Get(),
                uploadResource.Get(), 0, 0, 1, &subresourceData );

            // Add references to resources so they stay in scope until the command list is reset.
            TrackResource(uploadResource);
        }
        TrackResource(d3d12Resource);
    }

    buffer.SetD3D12Resource( d3d12Resource );
    buffer.CreateViews( numElements, elementSize );
}


void CommandList::CopyVertexBuffer( VertexBuffer& vertexBuffer, size_t numVertices, size_t vertexStride, const void* vertexBufferData )
{
    CopyBuffer( vertexBuffer, numVertices, vertexStride, vertexBufferData );
}

void CommandList::CopyIndexBuffer( IndexBuffer& indexBuffer, size_t numIndicies, DXGI_FORMAT indexFormat, const void* indexBufferData )
{
    size_t indexSizeInBytes = indexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4;
    CopyBuffer( indexBuffer, numIndicies, indexSizeInBytes, indexBufferData );
}

void CommandList::TransitionBarrier( Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES stateAfter,
                                     UINT subresource, bool flushBarriers )
{
    if ( resource )
    {
        // The "before" state is not important. It will be resolved by the resource state tracker.
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition( resource.Get(), D3D12_RESOURCE_STATE_COMMON, stateAfter,
                                                             subresource );
        m_ResourceStateTracker->ResourceBarrier( barrier );
    }

    if ( flushBarriers )
    {
        FlushResourceBarriers();
    }
}

void CommandList::TransitionBarrier( const Resource &resource, D3D12_RESOURCE_STATES stateAfter, UINT subResource,
                                     bool            flushBarriers )
{
    auto d3d12Resource = resource.GetD3D12Resource();
    if (d3d12Resource)
    {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12Resource.Get(), D3D12_RESOURCE_STATE_COMMON,
                                                            stateAfter, subResource);
        m_ResourceStateTracker->ResourceBarrier(barrier);
    }
    if (flushBarriers)
    {
        FlushResourceBarriers();
    }
}

void CommandList::UAVBarrier( const Resource &resource, bool flushBarriers )
{
    auto d3d12Resource = resource.GetD3D12Resource();
    if (d3d12Resource)
    {
        auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(d3d12Resource.Get());
        m_ResourceStateTracker->ResourceBarrier(barrier);
    }
    if (flushBarriers)
    {
        FlushResourceBarriers();
    }
}

void CommandList::AliasBarrier( const Resource &beforeResource, const Resource &afterResource, bool flushBarriers )
{
    auto beforeD3D12Resource = beforeResource.GetD3D12Resource();
    auto afterD3D12Resource = afterResource.GetD3D12Resource();
    auto barrier = CD3DX12_RESOURCE_BARRIER::Aliasing(beforeD3D12Resource.Get(), afterD3D12Resource.Get());
    m_ResourceStateTracker->ResourceBarrier(barrier);
    if (flushBarriers)
    {
        FlushResourceBarriers();
    }
}

void CommandList::ClearTexture( const Texture &texture, const float clearColor[4] )
{
    TransitionBarrier(texture, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_D3D12CommandList->ClearRenderTargetView(texture.GetRenderTargetView(), clearColor, 0, nullptr);

    TrackResource(texture);
}

void CommandList::ClearDepthStencilTexture( const Texture &texture, D3D12_CLEAR_FLAGS clearFlags, float depth,
    uint8_t stencil )
{
    TransitionBarrier(texture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    m_D3D12CommandList->ClearDepthStencilView(texture.GetDepthStencilView(), clearFlags, depth, stencil, 0, nullptr);

    TrackResource(texture);
}


void CommandList::SetDescriptorHeap( D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* descriptorHeap )
{
    if (m_DynamicDescriptors[heapType] != descriptorHeap)
    {
        m_DynamicDescriptors[heapType] = descriptorHeap;
        BindDescriptorHeaps();
    }
}
void CommandList::SetGraphicsDynamicStructuredBuffer( uint32_t slot, size_t numElements, size_t elementSize, const void* bufferData )
{
    size_t bufferSize = numElements * elementSize;

    auto heapAllocation = m_UploadBuffer->Allocate( bufferSize, elementSize );

    memcpy( heapAllocation.CPU, bufferData, bufferSize );

    m_D3D12CommandList->SetGraphicsRootShaderResourceView( slot, heapAllocation.GPU );
}

void CommandList::SetGraphics32BitConstants( uint32_t rootParameterIndex, uint32_t numConstants, const void* constants )
{
    m_D3D12CommandList->SetGraphicsRoot32BitConstants( rootParameterIndex, numConstants, constants, 0 );
}

void CommandList::SetViewport(const D3D12_VIEWPORT& viewport)
{
    SetViewports( {viewport} );
}

void CommandList::SetViewports(const std::vector<D3D12_VIEWPORT>& viewports)
{
    assert(viewports.size() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);
    m_D3D12CommandList->RSSetViewports( static_cast<UINT>( viewports.size() ),
        viewports.data() );
}

void CommandList::SetScissorRect(const D3D12_RECT& scissorRect)
{
    SetScissorRects({scissorRect});
}

void CommandList::SetScissorRects(const std::vector<D3D12_RECT>& scissorRects)
{
    assert( scissorRects.size() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);
    m_D3D12CommandList->RSSetScissorRects( static_cast<UINT>( scissorRects.size() ),
        scissorRects.data());
}

void CommandList::SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState)
{
    m_D3D12CommandList->SetPipelineState(pipelineState.Get());

    TrackResource(pipelineState);
}

void CommandList::SetGraphicsRootSignature( const RootSignature& rootSignature )
{
    auto d3d12RootSignature = rootSignature.GetRootSignature().Get();
    if ( m_RootSignature != d3d12RootSignature )
    {
        m_RootSignature = d3d12RootSignature;

        for ( int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i )
        {
            m_DynamicDescriptorHeap[i]->ParseRootSignature( rootSignature );
        }

        m_D3D12CommandList->SetGraphicsRootSignature(m_RootSignature);

        TrackResource(m_RootSignature);
    }
}

void CommandList::SetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY primitiveTopology ) const
{
    m_D3D12CommandList->IASetPrimitiveTopology( primitiveTopology );
}

void CommandList::SetComputeRootSignature( const RootSignature& rootSignature )
{
    auto d3d12RootSignature = rootSignature.GetRootSignature().Get();
    if ( m_RootSignature != d3d12RootSignature )
    {
        m_RootSignature = d3d12RootSignature;

        for ( int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i )
        {
            m_DynamicDescriptorHeap[i]->ParseRootSignature( rootSignature );
        }

        m_D3D12CommandList->SetComputeRootSignature(m_RootSignature);

        TrackResource(m_RootSignature);
    }
}
void CommandList::BindDescriptorHeaps()
{
    UINT                  numDescriptorHeaps = 0;
    ID3D12DescriptorHeap* descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};

    for (auto descriptorHeap: m_DynamicDescriptors)
    {
        if (descriptorHeap)
        {
            descriptorHeaps[numDescriptorHeaps++] = descriptorHeap;
        }
    }

    m_D3D12CommandList->SetDescriptorHeaps(numDescriptorHeaps, descriptorHeaps);
}


void CommandList::FlushResourceBarriers()
{
    m_ResourceStateTracker->FlushResourceBarriers(*this);
}

void CommandList::TrackObject( Microsoft::WRL::ComPtr<ID3D12Object> object )
{
    m_TrackedObjects.push_back(object);
}

void CommandList::TrackResource(Microsoft::WRL::ComPtr<ID3D12Object> object)
{
    m_TrackedObjects.push_back(object);
}
void CommandList::TrackResource( const Resource &resource )
{
    TrackObject(resource.GetD3D12Resource());
}

void CommandList::ReleaseTrackedObjects()
{
    m_TrackedObjects.clear();
}

void CommandList::ResolveSubresource( Resource& dstRes, const Resource& srcRes, uint32_t dstSubresource, uint32_t srcSubresource )
{
    TransitionBarrier( dstRes, D3D12_RESOURCE_STATE_RESOLVE_DEST, dstSubresource );
    TransitionBarrier( srcRes, D3D12_RESOURCE_STATE_RESOLVE_SOURCE, srcSubresource );

    FlushResourceBarriers();

    m_D3D12CommandList->ResolveSubresource( dstRes.GetD3D12Resource().Get(), dstSubresource, srcRes.GetD3D12Resource().Get(), srcSubresource, dstRes.GetD3D12ResourceDesc().Format );

    TrackResource( srcRes );
    TrackResource( dstRes );
}

void CommandList::LoadEmbeddedTexture( Texture* texture, const uint8_t* imageData, size_t size, const std::wstring &textureName)
{
    std::lock_guard<std::mutex> lock(ms_TextureCacheMutex);
    auto iter = ms_TextureCache.find(textureName);
    if (iter != ms_TextureCache.end())
    {
        texture->SetD3D12Resource(iter->second, nullptr);
        texture->CreateViews();
        texture->SetName(textureName);
    } else
    {
        DirectX::TexMetadata metadata{};
        DirectX::ScratchImage scratchImage;
        ThrowIfFailed(DirectX::LoadFromWICMemory(imageData, size, DirectX::WIC_FLAGS::WIC_FLAGS_NONE, &metadata, scratchImage));
        D3D12_RESOURCE_DESC textureDesc = {};
        switch (metadata.dimension)
        {
            case DirectX::TEX_DIMENSION_TEXTURE1D:
                textureDesc = CD3DX12_RESOURCE_DESC::Tex1D(
                    metadata.format,
                    static_cast<UINT64>(metadata.width),
                    static_cast<UINT16>(metadata.arraySize)
                );
                break;
            case DirectX::TEX_DIMENSION_TEXTURE2D:
                textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
                    metadata.format,
                    static_cast<UINT64>(metadata.width),
                    static_cast<UINT>(metadata.height),
                    static_cast<UINT16>(metadata.arraySize)
                );
                break;
            case DirectX::TEX_DIMENSION_TEXTURE3D:
                textureDesc = CD3DX12_RESOURCE_DESC::Tex3D(
                    metadata.format,
                    static_cast<UINT64>(metadata.width),
                    static_cast<UINT>(metadata.height),
                    static_cast<UINT16>(metadata.arraySize)
                );
            default:
                EE_CORE_ERROR("Invalid texture dimension.");
                throw std::exception("Invalid texture dimension");
        }
        auto                                   device = Renderer::Get()->GetDevice();
        Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;

        ThrowIfFailed(device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                D3D12_HEAP_FLAG_NONE,
                &textureDesc,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(&textureResource))
        );

        texture->SetD3D12Resource(textureResource, nullptr);
        texture->CreateViews();
        texture->SetName(textureName);

        ResourceStateTracker::AddGlobalResourceState(textureResource.Get(), D3D12_RESOURCE_STATE_COMMON);
        std::vector<D3D12_SUBRESOURCE_DATA> subresources(scratchImage.GetImageCount());
        const DirectX::Image*               pImages = scratchImage.GetImages();
        for (UINT i = 0; i < scratchImage.GetImageCount(); ++i)
        {
            auto &subresource = subresources[i];
            subresource.RowPitch = pImages[i].rowPitch;
            subresource.SlicePitch = pImages[i].slicePitch;
            subresource.pData = pImages[i].pixels;
        }

        CopyTextureSubresource(
            *texture,
            0,
            static_cast<uint32_t>(subresources.size()),
            subresources.data()
        );

        if (subresources.size() < textureResource->GetDesc().MipLevels)
        {
            // TODO: finish mips
            //GenerateMips(*texture);
        }
        ms_TextureCache[textureName] = textureResource.Get();
    }

}

void CommandList::LoadTextureFromFile( Texture &texture, const std::wstring &fileName, bool useSrgb )
{
    std::filesystem::path filePath(fileName);
    if (!std::filesystem::exists(filePath))
    {
        throw std::exception("File not found. ");
        EE_CORE_ERROR("Texture file not found");
    }

    std::lock_guard<std::mutex> lock(ms_TextureCacheMutex);
    auto                        iter = ms_TextureCache.find(fileName);
    if (iter != ms_TextureCache.end())
    {
        texture.SetD3D12Resource(iter->second, nullptr);
        texture.CreateViews();
        texture.SetName(fileName);
    } else
    {
        DirectX::TexMetadata  metadata{};
        DirectX::ScratchImage scratchImage;

        if (filePath.extension() == ".dds")
        {
            ThrowIfFailed(DirectX::LoadFromDDSFile(
                    fileName.c_str(),
                    DirectX::DDS_FLAGS_FORCE_RGB,
                    &metadata,
                    scratchImage)
            );
        } else if (filePath.extension() == "hdr")
        {
            ThrowIfFailed(DirectX::LoadFromHDRFile(
                    fileName.c_str(),
                    &metadata,
                    scratchImage)
            );
        } else if (filePath.extension() == "tga")
        {
            ThrowIfFailed(DirectX::LoadFromTGAFile(
                    fileName.c_str(),
                    &metadata,
                    scratchImage)
            );
        } else
        {
            ThrowIfFailed(DirectX::LoadFromWICFile(
                    fileName.c_str(),
                    DirectX::WIC_FLAGS_FORCE_RGB,
                    &metadata,
                    scratchImage)
            );
        }
        if (useSrgb)
        {
            metadata.format = DirectX::MakeSRGB(metadata.format);
        }
        D3D12_RESOURCE_DESC textureDesc = {};
        switch (metadata.dimension)
        {
            case DirectX::TEX_DIMENSION_TEXTURE1D:
                textureDesc = CD3DX12_RESOURCE_DESC::Tex1D(
                    metadata.format,
                    static_cast<UINT64>(metadata.width),
                    static_cast<UINT16>(metadata.arraySize)
                );
                break;
            case DirectX::TEX_DIMENSION_TEXTURE2D:
                textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
                    metadata.format,
                    static_cast<UINT64>(metadata.width),
                    static_cast<UINT>(metadata.height),
                    static_cast<UINT16>(metadata.arraySize)
                );
                break;
            case DirectX::TEX_DIMENSION_TEXTURE3D:
                textureDesc = CD3DX12_RESOURCE_DESC::Tex3D(
                    metadata.format,
                    static_cast<UINT64>(metadata.width),
                    static_cast<UINT>(metadata.height),
                    static_cast<UINT16>(metadata.arraySize)
                );
            default:
                EE_CORE_ERROR("Invalid texture dimension.");
                throw std::exception("Invalid texture dimension");
        }
        auto                                   device = Renderer::Get()->GetDevice();
        Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;

        ThrowIfFailed(device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                D3D12_HEAP_FLAG_NONE,
                &textureDesc,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(&textureResource))
        );

        texture.SetD3D12Resource(textureResource, nullptr);
        texture.CreateViews();
        texture.SetName(fileName);

        ResourceStateTracker::AddGlobalResourceState(textureResource.Get(), D3D12_RESOURCE_STATE_COMMON);
        std::vector<D3D12_SUBRESOURCE_DATA> subresources(scratchImage.GetImageCount());
        const DirectX::Image*               pImages = scratchImage.GetImages();
        for (UINT i = 0; i < scratchImage.GetImageCount(); ++i)
        {
            auto &subresource = subresources[i];
            subresource.RowPitch = pImages[i].rowPitch;
            subresource.SlicePitch = pImages[i].slicePitch;
            subresource.pData = pImages[i].pixels;
        }

        CopyTextureSubresource(
            texture,
            0,
            static_cast<uint32_t>(subresources.size()),
            subresources.data()
        );

        if (subresources.size() < textureResource->GetDesc().MipLevels)
        {
            GenerateMips(texture);
        }
        ms_TextureCache[fileName] = textureResource.Get();
    }
}

void CommandList::GenerateMips( Texture &texture )
{
    if (m_D3D12CommandListType == D3D12_COMMAND_LIST_TYPE_COPY )
    {
        if ( !m_ComputeCommandList )
        {
            m_ComputeCommandList = Renderer::Get()->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE)->GetCommandList();
        }
        m_ComputeCommandList->GenerateMips( texture);
    }
}

void CommandList::Close()
{
    FlushResourceBarriers();
    m_D3D12CommandList->Close();
}

bool CommandList::Close( CommandList &pendingCommandList )
{
    FlushResourceBarriers();
    m_D3D12CommandList->Close();
    uint32_t numPendingBarriers = m_ResourceStateTracker->FlushPendingResourceBarriers(pendingCommandList);

    m_ResourceStateTracker->CommitFinalResourceStates();

    return numPendingBarriers > 0;
}

void CommandList::Reset()
{
    ThrowIfFailed( m_D3D12CommandAllocator->Reset() );
    ThrowIfFailed( m_D3D12CommandList->Reset( m_D3D12CommandAllocator.Get(), nullptr ) );

    m_ResourceStateTracker->Reset();
    m_UploadBuffer->Reset();

    ReleaseTrackedObjects();

    for ( int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i )
    {
        m_DynamicDescriptorHeap[i]->Reset();
        m_DynamicDescriptors[i] = nullptr;
    }

    m_RootSignature = nullptr;
    m_ComputeCommandList = nullptr;
}


void CommandList::SetIndexBuffer( const IndexBuffer& indexBuffer )
{
    TransitionBarrier( indexBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER );

    auto indexBufferView = indexBuffer.GetIndexBufferView();

    m_D3D12CommandList->IASetIndexBuffer( &indexBufferView );

    TrackResource(indexBuffer);
}


void CommandList::SetVertexBuffer( uint32_t slot, VertexBuffer& vertexBuffer )
{
    TransitionBarrier( vertexBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER );

    auto vertexBufferView = vertexBuffer.GetVertexBufferView();

    m_D3D12CommandList->IASetVertexBuffers( slot, 1, &vertexBufferView );

    TrackResource(vertexBuffer);
}

}
