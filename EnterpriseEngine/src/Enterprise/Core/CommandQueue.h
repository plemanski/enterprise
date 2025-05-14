//
// Created by Peter on 5/10/2025.
//

#ifndef COMMANDQUEUE_H
#define COMMANDQUEUE_H
#include <cstdint>
#include <memory>
#include <queue>


#include <wrl/client.h>

#include "directx/d3d12.h"


namespace Enterprise::Core::Graphics {
class CommandList;

class CommandQueue {
public:

    CommandQueue(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
    virtual ~CommandQueue() = default;

    std::shared_ptr<CommandList> GetCommandList();


    uint64_t ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList);

    uint64_t Signal();
    bool IsFenceComplete(uint64_t fenceValue);
    void WaitForFenceValue(uint64_t fenceValue);
    void Flush();

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetD3D12CommandQueue() const;

protected:

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator() const;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CreateCommandList(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator) const;

private:
    // Keep track of command allocators that are "in-flight"
    struct CommandAllocatorEntry
    {
        uint64_t fenceValue;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
    };

    using CommandAllocatorQueue = std::queue<CommandAllocatorEntry>;
    using CommandListQueue = std::queue< Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> >;

    D3D12_COMMAND_LIST_TYPE                     m_CommandListType;
    Microsoft::WRL::ComPtr<ID3D12Device2>       m_d3d12Device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>  m_d3d12CommandQueue;
    Microsoft::WRL::ComPtr<ID3D12Fence>         m_d3d12Fence;
    HANDLE                                      m_FenceEvent;
    uint64_t                                    m_FenceValue;

    CommandAllocatorQueue                       m_CommandAllocatorQueue;
    CommandListQueue                            m_CommandListQueue;

};

}

#endif //COMMANDQUEUE_H
