//
// Created by Peter on 5/14/2025.
//

#include "VertexBuffer.h"

namespace Enterprise::Core::Graphics {

VertexBuffer::VertexBuffer(const std::wstring& name)
    : Buffer(name)
    , m_NumVertices(0)
    , m_VertexStride(0)
    , m_VertexBufferView({})
{}

VertexBuffer::~VertexBuffer()
{}

void VertexBuffer::CreateViews(size_t numElements, size_t elementSize)
{
    m_NumVertices = numElements;
    m_VertexStride = elementSize;

    m_VertexBufferView.BufferLocation = m_D3D12Resource->GetGPUVirtualAddress();
    m_VertexBufferView.SizeInBytes = static_cast<UINT>(m_NumVertices * m_VertexStride);
    m_VertexBufferView.StrideInBytes = static_cast<UINT>(m_VertexStride);
}

D3D12_CPU_DESCRIPTOR_HANDLE VertexBuffer::GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const
{
    throw std::exception("VertexBuffer::GetShaderResourceView should not be called.");
}

D3D12_CPU_DESCRIPTOR_HANDLE VertexBuffer::GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc) const
{
    throw std::exception("VertexBuffer::GetUnorderedAccessView should not be called.");
}

}