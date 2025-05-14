//
// Created by Peter on 5/12/2025.
//

#include <filesystem>

#include "CommandList.h"


#include "DirectXTex.h"
#include "DynamicDescriptorHeap.h"
#include "Log.h"
#include "Resource.h"
#include "ResourceStateTracker.h"
#include "UploadBuffer.h"


namespace Enterprise::Core::Graphics {
CommandList::CommandList( D3D12_COMMAND_LIST_TYPE type )
    : m_D3D12CommandListType(type)
{
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

void CommandList::CopyResource( Resource &destResource, const Resource &srcResource )
{
    TransitionBarrier(destResource, D3D12_RESOURCE_STATE_COPY_DEST);
    TransitionBarrier(srcResource, D3D12_RESOURCE_STATE_COPY_SOURCE);

    m_D3D12CommandList->CopyResource(destResource.GetD3D12Resource().Get(), srcResource.GetD3D12Resource().Get());
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

void CommandList::CopyBuffer( size_t               numElements, size_t elementSize, const void* bufferData,
                              D3D12_RESOURCE_FLAGS flags )
{
    auto   device = Renderer::Get()->GetDevice();
    size_t bufferSize = numElements * elementSize;

    Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource;

    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags),
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&d3d12Resource)));

    ResourceStateTracker::AddGlobalResourceState(d3d12Resource.Get(), D3D12_RESOURCE_STATE_COMMON);

    if (bufferData)
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> intResource;
        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&intResource)));

        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = bufferData;
        subresourceData.RowPitch = bufferSize;
        subresourceData.SlicePitch = subresourceData.RowPitch;

        m_ResourceStateTracker->TransitionResource(d3d12Resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST);

        UpdateSubresources(m_D3D12CommandList.Get(),
                           d3d12Resource.Get(), intResource.Get(),
                           0, 0, 1, &subresourceData);
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


void CommandList::SetDescriptorHeap( D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* descriptorHeap )
{
    if (m_DynamicDescriptors[heapType] != descriptorHeap)
    {
        m_DynamicDescriptors[heapType] = descriptorHeap;
        BindDescriptorHeaps();
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

void CommandList::TrackResource( const Resource &resource )
{
    TrackObject(resource.GetD3D12Resource());
}

void CommandList::ReleaseTrackedObjects()
{
    m_TrackedObjects.clear();
}

void CommandList::LoadTextureFromFile( Texture &texture, const std::wstring &fileName, bool useSrgb )
{
    std::filesystem::path filePath(fileName);
    if (!std::filesystem::exists(filePath))
    {
        throw std::exception("File not found. ");
        EE_CORE_ERROR("Texture file not found: {}", &fileName);
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
}
