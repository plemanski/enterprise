/**
 *  @file VertexBuffer.h
 *  @date October 24, 2018
 *  @author Jeremiah van Oosten
 *
 *  @brief Vertex buffer resource.
 */



#ifndef VERTEXBUFFER_H
#define VERTEXBUFFER_H
#include "Buffer.h"
#include "../Core.h"


namespace Enterprise::Core::Graphics {

class ENTERPRISE_API VertexBuffer : public Buffer
{
public:
    VertexBuffer(const std::wstring& name = L"");
    virtual ~VertexBuffer();

    // Inherited from Buffer
    void CreateViews(size_t numElements, size_t elementSize) override;

    /**
     * Get the vertex buffer view for binding to the Input Assembler stage.
     */
    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const
    {
        return m_VertexBufferView;
    }

    size_t GetNumVertices() const
    {
        return m_NumVertices;
    }

    size_t GetVertexStride() const
    {
        return m_VertexStride;
    }

    /**
    * Get the SRV for a resource.
    */
    D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const override;

    /**
    * Get the UAV for a (sub)resource.
    */
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const override;

protected:

private:
    size_t m_NumVertices;
    size_t m_VertexStride;

    D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
};;

}


#endif //VERTEXBUFFER_H
