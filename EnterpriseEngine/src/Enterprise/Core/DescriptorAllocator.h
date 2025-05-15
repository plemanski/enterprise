//
// Created by Peter on 5/12/2025.
//

#ifndef DESCRIPTORALLOCATOR_H
#define DESCRIPTORALLOCATOR_H

#include "directx/d3d12.h"
#include "directx/d3dx12.h"


#include <cstdint>
#include <mutex>
#include <memory>
#include <set>
#include <vector>

#include "Renderer.h"
#include "DescriptorAllocation.h"
#include "Core.h"

namespace Enterprise::Core::Graphics {

class DescriptorAllocatorPage;

class ENTERPRISE_API DescriptorAllocator {
public:
    DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsPerHeap = 256);
    //virtual ~DescriptorAllocator();

    DescriptorAllocation Allocate(uint32_t numDescriptors = 1);

    void ReleaseStaleDescriptors( uint64_t frameNumber );

private:
    using DescriptorHeapPool = std::vector< std::shared_ptr<DescriptorAllocatorPage> >;

    std::shared_ptr<DescriptorAllocatorPage> CreateAllocatorPage();

    D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType;
    uint32_t m_NumDescriptorsPerHeap;

    DescriptorHeapPool m_HeapPool;
    std::set<size_t> m_AvailableHeaps;

    std::mutex m_AllocationMutex;
};

}

#endif //DESCRIPTORALLOCATOR_H
