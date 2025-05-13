//
// Created by Peter on 5/8/2025.
//

#ifndef DYNAMICDESCRIPTORHEAPH
#define DYNAMICDESCRIPTORHEAPH

#include "directx/d3dx12.h"

#include <wrl/client.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <queue>


namespace Enterprise::Core::Graphics {

class CommandList;
class RootSignature;

class DynamicDescriptorHeap {
public:
    DynamicDescriptorHeap(
        D3D12_DESCRIPTOR_HEAP_TYPE heapType,
        uint32_t numDescriptorsPerHeap = 1024);

    virtual ~DynamicDescriptorHeap();

    void StageDescriptors(uint32_t rootParameterIndex, uint32_t offset, uint32_t numDescriptors,
        D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor);

    /**
     * Copy all of the staged descriptors to the GPU visible descriptor heap and
     * bind the descriptor heap and the descriptor tables to the command list.
     * The passed-in function object is used to set the GPU visible descriptors
     * on the command list. Two possible functions are:
     *   * Before a draw    : ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable
     *   * Before a dispatch: ID3D12GraphicsCommandList::SetComputeRootDescriptorTable
     *
     * Since the DynamicDescriptorHeap can't know which function will be used, it must
     * be passed as an argument to the function.
     */
    void CommitStagedDescriptors( CommandList& commandList,
        std::function<void(ID3D12GraphicsCommandList*, UINT, D3D12_GPU_DESCRIPTOR_HANDLE)> setFunc );
    void CommitStagedDescriptorsForDraw( CommandList& commandList );
    void CommitStagedDescriptorsForCompute( CommandList& commandList );

    /**
     * Copies a single CPU visible descriptor to a GPU visible descriptor heap.
     * This is useful for the
     *   * ID3D12GraphicsCommandList::ClearUnorderedAccessViewFloat
     *   * ID3D12GraphicsCommandList::ClearUnorderedAccessViewUint
     * methods which require both a CPU and GPU visible descriptors for a UAV
     * resource.
     *
     * @param commandList The command list is required in case the GPU visible
     * descriptor heap needs to be updated on the command list.
     * @param cpuDescriptor The CPU descriptor to copy into a GPU visible
     * descriptor heap.
     *
     * @return The GPU visible descriptor.
    */
    D3D12_GPU_DESCRIPTOR_HANDLE CopyDescriptor( CommandList& commandList, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor);

    void ParseRootSignature( const RootSignature& rootSignature );

    void Reset();

private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> RequestDescriptorHeap();
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap();

    [[nodiscard]] uint32_t ComputeStaleDescriptorCount() const;

    static constexpr uint32_t MaxDescriptorTables = 32;

    struct DescriptorTableCache {
        DescriptorTableCache()
            : NumDescriptors( 0 )
            , BaseDescriptor( nullptr )
        {}

        void Reset()
        {
            NumDescriptors = 0;
            BaseDescriptor = nullptr;
        }

        uint32_t NumDescriptors;
        D3D12_CPU_DESCRIPTOR_HANDLE* BaseDescriptor;
    };

    using DescriptorHeapPool = std::queue< Microsoft::WRL::ComPtr< ID3D12DescriptorHeap> >;

    D3D12_DESCRIPTOR_HEAP_TYPE                      m_DescriptorHeapType;
    uint32_t                                        m_NumDescriptorsPerHeap;
    uint32_t                                        m_DescriptorHandleIncrementSize;
    std::unique_ptr<D3D12_CPU_DESCRIPTOR_HANDLE[]>  m_DescriptorHandleCache;
    DescriptorTableCache                            m_DescriptorTableCache[MaxDescriptorTables];
    uint32_t                                        m_DescriptorTableBitMask;
    uint32_t                                        m_StaleDescriptorTableBitMask;
    DescriptorHeapPool                              m_DescriptorHeapPool;
    DescriptorHeapPool                              m_AvailableDescriptorHeaps;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>    m_CurrentDescriptorHeap;
    CD3DX12_GPU_DESCRIPTOR_HANDLE                   m_CurrentGPUDescriptorHandle;
    CD3DX12_CPU_DESCRIPTOR_HANDLE                   m_CurrentCPUDescriptorHandle;
    uint32_t                                        m_NumFreeHandles;
};

}

#endif

