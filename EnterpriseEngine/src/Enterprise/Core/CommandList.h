//
// Created by Peter on 5/12/2025.
//

#ifndef COMMANDLIST_H
#define COMMANDLIST_H
#include <cstdint>

#include "directx/d3d12.h"



namespace  Enterprise::Core::Engine {

class Resource;
class CommandList {
public:

    void SetGraphicsDynamicConstantBuffer( uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData );

    void SetShaderResourceView( uint32_t rootParameterIndex, uint32_t descriptorOffset,
        const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT firstSubResoruce, UINT numSubResources,
        const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc);

    void Draw( uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance );
    void CopyResource ( Resource& dstResource, const Resource& srcResrource );
    void TransitionBarrier( const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT subResource, bool flushBarriers);
    void SetDescriptorHeap( D3D12_DESCRIPTOR_HEAP_TYPE, ID3D12DescriptorHeap& descriptorHeap );
    void FlushResourceBarriers();
};


}

#endif //COMMANDLIST_H
