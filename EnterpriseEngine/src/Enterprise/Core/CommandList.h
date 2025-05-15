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

#include "IndexBuffer.h"
#include "RenderTarget.h"
#include "RootSignature.h"
#include "VertexBuffer.h"
#include "directx/d3d12.h"
#include "UploadBuffer.h"
#include "DynamicDescriptorHeap.h"
#include "ResourceStateTracker.h"


namespace Enterprise::Core::Graphics {

class ENTERPRISE_API CommandList {
public:
    CommandList( D3D12_COMMAND_LIST_TYPE type );

    void SetGraphicsDynamicConstantBuffer( uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData );

    void SetShaderResourceView( uint32_t rootParameterIndex, uint32_t descriptorOffset,
                                const Resource &resource, D3D12_RESOURCE_STATES stateAfter, UINT firstSubResource = 0,
                                UINT numSubResources = 0,
                                const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr );

    void SetRenderTarget( const RenderTarget &renderTarget );

    [[nodiscard]] Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> GetGraphicsCommandList() const
    {
        return m_D3D12CommandList;
    }

    void Draw( uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance );

    void DrawIndexed( uint32_t indexCountPerInstance, uint32_t instanceCount = 1,
                      uint32_t startIndex = 0, uint32_t        startVertex = 0, uint32_t startInstance = 0 );

    void CopyResource( Resource &destResource, const Resource &srcResource );

    void CopyTextureSubresource( const Texture &         texture, uint32_t firstSubResource, uint32_t numSubResources,
                                 D3D12_SUBRESOURCE_DATA* data );

    // Copy the contents of a CPU buffer to a GPU buffer (possibly replacing the previous buffer contents).
    void CopyBuffer( Buffer &             buffer, size_t numElements, size_t elementSize, const void* bufferData,
                     D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE );

    /**
     * Copy the contents to a vertex buffer in GPU memory.
     */
    void CopyVertexBuffer( VertexBuffer &vertexBuffer, size_t numVertices, size_t vertexStride,
                           const void*   vertexBufferData );

    template<typename T>
    void CopyVertexBuffer( VertexBuffer &vertexBuffer, const std::vector<T> &vertexBufferData )
    {
        CopyVertexBuffer(vertexBuffer, vertexBufferData.size(), sizeof(T), vertexBufferData.data());
    }

    /**
     * Copy the contents to a index buffer in GPU memory.
     */
    void CopyIndexBuffer( IndexBuffer &indexBuffer, size_t numIndicies, DXGI_FORMAT indexFormat,
                          const void*  indexBufferData );

    template<typename T>
    void CopyIndexBuffer( IndexBuffer &indexBuffer, const std::vector<T> &indexBufferData )
    {
        assert(sizeof( T ) == 2 || sizeof( T ) == 4);

        DXGI_FORMAT indexFormat = (sizeof(T) == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
        CopyIndexBuffer(indexBuffer, indexBufferData.size(), indexFormat, indexBufferData.data());
    }

    void TransitionBarrier( const Resource &resource, D3D12_RESOURCE_STATES stateAfter,
                            UINT subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false );

    void UAVBarrier( const Resource &resource, bool flushBarriers = false );

    void AliasBarrier( const Resource &beforeResource, const Resource &afterResource, bool flushBarriers = false );

    /**
     * Clear a texture.
     */
    void ClearTexture( const Texture &texture, const float clearColor[4] );

    /**
     * Clear depth/stencil texture.
     */
    void ClearDepthStencilTexture( const Texture &texture, D3D12_CLEAR_FLAGS clearFlags, float depth = 1.0f,
                                   uint8_t        stencil = 0 );

    void SetDescriptorHeap( D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* descriptorHeap );

    void SetGraphicsDynamicStructuredBuffer( uint32_t    slot, size_t numElements, size_t elementSize,
                                             const void* bufferData );

    /**
     * Set a set of 32-bit constants on the graphics pipeline.
     */
    void SetGraphics32BitConstants( uint32_t rootParameterIndex, uint32_t numConstants, const void* constants );

    template<typename T>
    void SetGraphics32BitConstants( uint32_t rootParameterIndex, const T &constants )
    {
        static_assert(sizeof(T) % sizeof(uint32_t) == 0, "Size of type must be a multiple of 4 bytes");
        SetGraphics32BitConstants(rootParameterIndex, sizeof(T) / sizeof(uint32_t), &constants);
    }

    void SetViewport( const D3D12_VIEWPORT &viewport );

    void SetViewports( const std::vector<D3D12_VIEWPORT> &viewports );

    void SetScissorRect( const D3D12_RECT &scissorRect );

    void SetScissorRects( const std::vector<D3D12_RECT> &scissorRects );

    void SetPipelineState( Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState );

    void SetGraphicsRootSignature( const RootSignature &rootSignature );

    void SetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY primitiveTopology ) const;

    void SetComputeRootSignature( const RootSignature &rootSignature );

    void BindDescriptorHeaps();

    void FlushResourceBarriers();

    void TrackObject( Microsoft::WRL::ComPtr<ID3D12Object> object );

    void TrackResource( Microsoft::WRL::ComPtr<ID3D12Object> object );

    void TrackResource( const Resource &resource );

    void ReleaseTrackedObjects();

    void ResolveSubresource( Resource &dstRes, const Resource &srcRes, uint32_t dstSubresource = 0,
                             uint32_t  srcSubresource = 0 );

    void LoadTextureFromFile( Texture &texture, const std::wstring &fileName, bool useSrgb );

    std::shared_ptr<CommandList> GetGenerateMipsCommandList() const { return m_ComputeCommandList; }

    void GenerateMips( Texture &texture );

    void Close();

    bool Close( CommandList &pendingCommandList );

    void Reset();

    void SetVertexBuffer( uint32_t slot, VertexBuffer &vertexBuffer );

    void SetIndexBuffer( const IndexBuffer &indexBuffer );

private:
    std::unique_ptr<ResourceStateTracker>              m_ResourceStateTracker;
    std::unique_ptr<DynamicDescriptorHeap>             m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    std::unique_ptr<UploadBuffer>                      m_UploadBuffer;
    std::shared_ptr<CommandList>                       m_ComputeCommandList;
    ID3D12RootSignature*                               m_RootSignature;
    ID3D12DescriptorHeap*                              m_DynamicDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    D3D12_COMMAND_LIST_TYPE                            m_D3D12CommandListType;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> m_D3D12CommandList;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>     m_D3D12CommandAllocator;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Object> > m_TrackedObjects;
    static std::map<std::wstring, ID3D12Resource *>    ms_TextureCache;
    static std::mutex                                  ms_TextureCacheMutex;
};
}

#endif //COMMANDLIST_H
