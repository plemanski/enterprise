//
// Created by Peter on 5/12/2025.
//

#include "UploadBuffer.h"

namespace Enterprise::Core::Graphics {

UploadBuffer::UploadBuffer(size_t pageSize)
    : m_PageSize(pageSize)
{}

UploadBuffer::Allocation UploadBuffer::Allocate(size_t sizeInBytes, size_t alignment)
{
    if (sizeInBytes > m_PageSize)
    {
        throw std::bad_alloc();
    }

    if (!m_CurrentPage || !m_CurrentPage->HasSpace(sizeInBytes, alignment))
    {
        m_CurrentPage = RequestPage();
    }

    return m_CurrentPage->Allocate(sizeInBytes, alignment);
}

std::shared_ptr<UploadBuffer::Page> UploadBuffer::RequestPage()
{
    std::shared_ptr<Page> page;

    if (!m_AvailablePages.empty())
    {
        page = m_AvailablePages.front();
        m_AvailablePages.pop_front();
    } else
    {
        page = std::make_shared<Page>(m_PageSize);
        m_PagePool.push_back(page);
    }

    return page;
}

void UploadBuffer::Reset()
{
    m_CurrentPage = nullptr;
    m_AvailablePages = m_PagePool;

   for (auto page : m_AvailablePages )
   {
       page->Reset();
   }
}

UploadBuffer::Page::Page(size_t sizeInBytes)
    : m_PageSize(sizeInBytes)
    , m_Offset(0)
    , m_pCPU(nullptr)
    , m_pGPU(D3D12_GPU_VIRTUAL_ADDRESS(0))
{
    auto device = Renderer::Get()->GetDevice();

    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(m_PageSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_D3D12Resource)
    ));

    m_pGPU = m_D3D12Resource->GetGPUVirtualAddress();
    m_D3D12Resource->Map(0, nullptr, &m_pCPU);
}

UploadBuffer::Page::~Page()
{
    m_D3D12Resource->Unmap(0, nullptr);
    m_pCPU = nullptr;
    m_pGPU = D3D12_GPU_VIRTUAL_ADDRESS(0);
}

// Does the page have enough space, and can the requested alignment be satisfied
bool UploadBuffer::Page::HasSpace(size_t sizeInBytes, size_t alignment) const
{
    size_t alignedSize = AlignUp(sizeInBytes, alignment);
    size_t alignedOffset = AlignUp(m_Offset, alignment);

    return alignedOffset + alignedSize <= m_PageSize;
}

UploadBuffer::Allocation UploadBuffer::Page::Allocate(size_t sizeInBytes, size_t alignment)
{
    if (!HasSpace(sizeInBytes, alignment))
    {
        throw std::bad_alloc();
    }

    size_t alignedSize = AlignUp(sizeInBytes, alignment);
    m_Offset = AlignUp(m_Offset, alignment);

    Allocation allocation{};
    allocation.CPU = static_cast<uint8_t*>(m_pCPU) + m_Offset;
    allocation.GPU = m_pGPU + m_Offset;

    m_Offset += alignedSize;

    return allocation;

}

void UploadBuffer::Page::Reset()
{
    m_Offset = 0;
}



}
