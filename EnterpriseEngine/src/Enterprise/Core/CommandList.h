//
// Created by Peter on 5/12/2025.
//

#ifndef COMMANDLIST_H
#define COMMANDLIST_H
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <wrl/client.h>
#include "directx/d3d12.h"


namespace Enterprise::Core::Graphics {
class DynamicDescriptorHeap;
class ResourceStateTracker;
class UploadBuffer;

class Resource;
class Texture;

class CommandList {
public:
    CommandList( D3D12_COMMAND_LIST_TYPE type );

    void SetGraphicsDynamicConstantBuffer( uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData );

    void SetShaderResourceView( uint32_t rootParameterIndex, uint32_t descriptorOffset,
                                const Resource &resource, D3D12_RESOURCE_STATES stateAfter, UINT firstSubResource = 0,
                                UINT numSubResources = 0,
                                const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr );

    [[nodiscard]] Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> GetGraphicsCommandList() const
    {
        return m_D3D12CommandList;
    }

    void Draw( uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance );

    void DrawIndexed( uint32_t indexCountPerInstance, uint32_t instanceCount,
                      uint32_t startIndex, uint32_t            startVertex, uint32_t startInstance );

    void CopyResource( Resource &destResource, const Resource &srcResource );

    void CopyTextureSubresource ( const Texture& texture, uint32_t firstSubResource, uint32_t numSubResources, D3D12_SUBRESOURCE_DATA* data );

    void CopyBuffer( size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags );

    void TransitionBarrier( const Resource &resource, D3D12_RESOURCE_STATES stateAfter,
                            UINT subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false );

    void UAVBarrier( const Resource &resource, bool flushBarriers = false );

    void AliasBarrier( const Resource &beforeResource, const Resource &afterResource, bool flushBarriers = false );

    void SetDescriptorHeap( D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* descriptorHeap );

    void BindDescriptorHeaps();

    void FlushResourceBarriers();

    void TrackObject( Microsoft::WRL::ComPtr<ID3D12Object> object );

    void TrackResource( const Resource &resource );

    void ReleaseTrackedObjects();

    void LoadTextureFromFile( Texture &texture, const std::wstring &fileName, bool useSrgb );

    std::shared_ptr<CommandList> GetGenerateMipsCommandList() const { return m_ComputeCommandList; }

    void GenerateMips( Texture& texture );

    void Close();

    bool Close( CommandList &pendingCommandList );

private:
    std::unique_ptr<UploadBuffer>                      m_UploadBuffer;
    ID3D12RootSignature*                               m_RootSignature;
    std::unique_ptr<ResourceStateTracker>              m_ResourceStateTracker;
    std::unique_ptr<DynamicDescriptorHeap>             m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    ID3D12DescriptorHeap*                              m_DynamicDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> m_D3D12CommandList;
    std::shared_ptr<CommandList>                       m_ComputeCommandList;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Object> > m_TrackedObjects;
    D3D12_COMMAND_LIST_TYPE                            m_D3D12CommandListType;
    static std::map<std::wstring, ID3D12Resource *>    ms_TextureCache;
    static std::mutex                                  ms_TextureCacheMutex;
};
}

#endif //COMMANDLIST_H
