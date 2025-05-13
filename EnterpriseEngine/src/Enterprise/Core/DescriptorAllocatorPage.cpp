//
// Created by Peter on 5/12/2025.
//

#include "DescriptorAllocatorPage.h"
#include "Renderer.h"

namespace Enterprise::Core::Graphics {

DescriptorAllocatorPage::DescriptorAllocatorPage( D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors )
    : m_HeapType( type )
    , m_NumDescriptorsInHeap( numDescriptors )
{
    auto device = Renderer::Get()->GetDevice();

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = m_HeapType;
    heapDesc.NumDescriptors = m_NumDescriptorsInHeap;

    ThrowIfFailed( device->CreateDescriptorHeap( &heapDesc, IID_PPV_ARGS(&m_D3D12DescriptorHeap) ) );

    m_BaseDescriptor = m_D3D12DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    m_DescriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize( m_HeapType );
    m_NumFreeHandles = m_NumDescriptorsInHeap;

    AddNewBlock( 0, m_NumFreeHandles );
}

D3D12_DESCRIPTOR_HEAP_TYPE DescriptorAllocatorPage::GetHeapType() const
{
    return m_HeapType;
}

uint32_t DescriptorAllocatorPage::NumFreeHandles() const
{
    return m_NumFreeHandles;
}

bool DescriptorAllocatorPage::HasSpace(uint32_t numDescriptors) const
{
    return m_FreeListBySize.lower_bound(numDescriptors) != m_FreeListBySize.end();
}

void DescriptorAllocatorPage::AddNewBlock( uint32_t offset, uint32_t numDescriptors )
{
    auto offsetIter = m_FreeListByOffset.emplace(offset, numDescriptors );
    auto sizeIter = m_FreeListBySize.emplace( numDescriptors, offsetIter.first );
    offsetIter.first->second.FreeListBySizeIter = sizeIter;
}

DescriptorAllocation DescriptorAllocatorPage::Allocate( uint32_t numDescriptors )
{
    std::lock_guard<std::mutex> lock(m_AllocationMutex);

    if (numDescriptors > m_NumFreeHandles )
    {
        return DescriptorAllocation();
    }

    // Get the first block large enough to satisfy the request
    auto smallestBlockIter = m_FreeListBySize.lower_bound( numDescriptors );
    if ( smallestBlockIter == m_FreeListBySize.end() )
    {
        return DescriptorAllocation();
    }

    auto blockSize = smallestBlockIter->first;

    auto offsetIter = smallestBlockIter->second;

    auto offset = offsetIter->first;

    // Remove the existing free block from the free list.
    m_FreeListBySize.erase( smallestBlockIter );
    m_FreeListByOffset.erase( offsetIter );

    // Compute the new free block that results from splitting this block.
    auto newOffset = offset + numDescriptors;
    auto newSize = blockSize - numDescriptors;

    if ( newSize > 0 )
    {
        // Add left-over to the free list if it doesn't exactly match the requested size
        AddNewBlock( newOffset, newSize );
    }

    m_NumFreeHandles -= numDescriptors;

    return DescriptorAllocation(
        CD3DX12_CPU_DESCRIPTOR_HANDLE(m_BaseDescriptor, offset, m_DescriptorHandleIncrementSize),
        numDescriptors, m_DescriptorHandleIncrementSize, shared_from_this() );
}

void DescriptorAllocatorPage::Free( DescriptorAllocation&& descriptor, uint64_t frameNumber )
{
    auto offset = ComputeOffset( descriptor.GetDescriptorHandle() );

    std::lock_guard<std::mutex> lock( m_AllocationMutex );

    m_StaleDescriptors.emplace( offset, descriptor.GetNumHandles(), frameNumber );
}

// Case 1: Free block immediately preceding the block being freed
// Case 2: Free block immediately following the block being freed
// Case 3: Free blocks in both positions
// Case 4: No blocks in either position

// If case 1 is true, merge the previous block
// Case 2 is true, merge the next block
// Case 3 and 4 are implicitely handled
void DescriptorAllocatorPage::FreeBlock(uint32_t offset, uint32_t numDescriptors)
{
    // Find the first element whose offset is greater than the specified offset.
    // This is the block that should appear after the block that is being freed.
    auto nextBlockIter = m_FreeListByOffset.upper_bound( offset );

    auto prevBlockIter = nextBlockIter;
    if ( prevBlockIter != m_FreeListByOffset.begin() )
    {
        // If the preceding block is not the first in the list, then go the previous block
        --prevBlockIter;
    } else
    {
        // Otherwise no block comes before the one to be free, so set to the end of the list.
        prevBlockIter = m_FreeListByOffset.end();

    }

    // Add the number of free handles back to the heap.
    // This needs to be done before merging any blocks since merging
    // blocks modifies the numDescriptors variable.
    m_NumFreeHandles += numDescriptors;
    if (prevBlockIter != m_FreeListByOffset.end() &&
        offset == prevBlockIter->first + prevBlockIter->second.Size)
    {
        // The previous block is exactly behind the block that is to be freed.
        //
        // PrevBlock.Offset           Offset
        // |                          |
        // |<-----PrevBlock.Size----->|<------Size-------->|
        //

        // Increase the block size by the size of merging with the prev block
        offset = prevBlockIter->first;
        numDescriptors += prevBlockIter->second.Size;

        // Remove the previous block from the free list
        m_FreeListBySize.erase( prevBlockIter->second.FreeListBySizeIter );
        m_FreeListByOffset.erase( prevBlockIter );
    } // Case 1 handled

    if (nextBlockIter != m_FreeListByOffset.end() &&
        offset + numDescriptors == nextBlockIter->first )
    {
        // The next block is exactly in front of the block that is to be freed.
        //
        // Offset               NextBlock.Offset
        // |                    |
        // |<------Size-------->|<-----NextBlock.Size----->|

        numDescriptors += nextBlockIter->second.Size;

        m_FreeListBySize.erase( nextBlockIter->second.FreeListBySizeIter );
        m_FreeListByOffset.erase( nextBlockIter );
    } // Case 2 handled

    AddNewBlock( offset, numDescriptors );
}

void DescriptorAllocatorPage::ReleaseStaleDescriptors(uint64_t frameNumber)
{
    std::lock_guard<std::mutex> lock( m_AllocationMutex );

    while ( !m_StaleDescriptors.empty() && m_StaleDescriptors.front().FrameNumber <= frameNumber )
    {
        auto& staleDescriptor = m_StaleDescriptors.front();

        auto offset = staleDescriptor.Offset;
        auto numDescriptor = staleDescriptor.Size;

        m_StaleDescriptors.pop();
    }
}

uint32_t DescriptorAllocatorPage::ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    return static_cast<uint32_t>( handle.ptr - m_BaseDescriptor.ptr ) / m_DescriptorHandleIncrementSize;
}
}
