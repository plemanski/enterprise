//
// Created by Peter on 5/12/2025.
//

#define NOMINMAX
#include "DescriptorAllocator.h"
#include "DescriptorAllocatorPage.h"

namespace Enterprise::Core::Graphics {
DescriptorAllocator::DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsPerHeap)
    : m_HeapType(type)
    , m_NumDescriptorsPerHeap( numDescriptorsPerHeap )
{}

std::shared_ptr<DescriptorAllocatorPage> DescriptorAllocator::CreateAllocatorPage()
{
    auto newPage = std::make_shared<DescriptorAllocatorPage>( m_HeapType, m_NumDescriptorsPerHeap );

    m_HeapPool.emplace_back( newPage );
    m_AvailableHeaps.insert( m_HeapPool.size() -1 );

    return newPage;
}

DescriptorAllocation DescriptorAllocator::Allocate(uint32_t numDescriptors)
{
    std::lock_guard<std::mutex> lock( m_AllocationMutex );

    DescriptorAllocation allocation;

    auto heap = m_AvailableHeaps.begin();
    while (heap != m_AvailableHeaps.end())
    {
        auto allocatorPage = m_HeapPool[*heap];

        allocation = allocatorPage->Allocate( numDescriptors );

        if ( allocatorPage->NumFreeHandles() == 0 )
        {
            m_AvailableHeaps.erase( heap );
        } else
        {
            ++heap;
        }

        if (!allocation.IsNull())
        {
            break;
        }
    }

    if (allocation.IsNull() )
    {
        m_NumDescriptorsPerHeap = std::max( m_NumDescriptorsPerHeap, numDescriptors );
        auto newPage = CreateAllocatorPage();

        allocation = newPage ->Allocate( numDescriptors );
    }

    return allocation;
}

void DescriptorAllocator::ReleaseStaleDescriptors(uint64_t frameNumber)
{
    std::lock_guard<std::mutex> lock( m_AllocationMutex );

    for ( size_t i = 0; i < m_HeapPool.size(); ++i )
    {
        auto page = m_HeapPool[i];

        page->ReleaseStaleDescriptors( frameNumber );

        if (page->NumFreeHandles() > 0 )
        {
            m_AvailableHeaps.insert( i );
        }
    }
}



}
