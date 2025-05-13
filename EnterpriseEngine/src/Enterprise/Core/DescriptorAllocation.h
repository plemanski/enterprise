//
// Created by Peter on 5/12/2025.
//

#ifndef DESCRIPTORALLOCATION_H
#define DESCRIPTORALLOCATION_H


#include "directx/d3d12.h"

#include <cstdint>
#include <memory>



namespace Enterprise::Core::Graphics {
class DescriptorAllocatorPage;

class DescriptorAllocation {
public:
    DescriptorAllocation();

    DescriptorAllocation( D3D12_CPU_DESCRIPTOR_HANDLE descriptor,
        uint32_t numHandles, uint32_t descriptorSize,
        std::shared_ptr<DescriptorAllocatorPage> page );


    ~DescriptorAllocation();

    DescriptorAllocation( const DescriptorAllocation& ) = delete;
    DescriptorAllocation& operator=( const DescriptorAllocation& ) = delete;

    DescriptorAllocation( DescriptorAllocation&& allocation ) noexcept ;
    DescriptorAllocation& operator=( DescriptorAllocation&& other ) noexcept ;

    [[nodiscard]] bool IsNull() const;

    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle( uint32_t offset = 0 ) const;

    [[nodiscard]] uint32_t GetNumHandles() const;

    [[nodiscard]] std::shared_ptr<DescriptorAllocatorPage> GetDescriptorAllocatorPage() const;

private:
    void Free();

    D3D12_CPU_DESCRIPTOR_HANDLE                 m_Descriptor;
    uint32_t                                    m_NumHandles;
    uint32_t                                    m_DescriptorSize;
    std::shared_ptr<DescriptorAllocatorPage>    m_Page;
};

}

#endif //DESCRIPTORALLOCATION_H
