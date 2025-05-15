//
// Created by Peter on 5/10/2025.
//

#ifndef COMMANDQUEUE_H
#define COMMANDQUEUE_H
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <queue>
#include <atomic>


#include <wrl/client.h>

#include "Core.h"
#include "ThreadSafeQueue.h"
#include "directx/d3d12.h"


namespace Enterprise::Core::Graphics {
class CommandList;

class ENTERPRISE_API CommandQueue {
public:
    CommandQueue( D3D12_COMMAND_LIST_TYPE type );

    virtual ~CommandQueue();

    // Get an available command list from the command queue.
    std::shared_ptr<CommandList> GetCommandList();

    // Execute a command list.
    // Returns the fence value to wait for for this command list.
    uint64_t ExecuteCommandList( const std::shared_ptr<CommandList>& commandList );

    uint64_t ExecuteCommandLists( const std::vector<std::shared_ptr<CommandList> > &commandLists );

    uint64_t Signal();

    bool IsFenceComplete( uint64_t fenceValue );

    void WaitForFenceValue( uint64_t fenceValue );

    void Flush();

    // Wait for another command queue to finish.
    void Wait( const CommandQueue &other );

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetD3D12CommandQueue() const;

private:
    // Free any command lists that are finished processing on the command queue.
    void ProccessInFlightCommandLists();

    // Keep track of command allocators that are "in-flight"
    // The first member is the fence value to wait for, the second is the
    // a shared pointer to the "in-flight" command list.
    using CommandListEntry = std::tuple<uint64_t, std::shared_ptr<CommandList> >;

    D3D12_COMMAND_LIST_TYPE                    m_CommandListType;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_d3d12CommandQueue;
    Microsoft::WRL::ComPtr<ID3D12Fence>        m_d3d12Fence;
    std::atomic_uint64_t                       m_FenceValue;

    Threads::ThreadSafeQueue<CommandListEntry>              m_InFlightCommandLists;
    Threads::ThreadSafeQueue<std::shared_ptr<CommandList> > m_AvailableCommandLists;

    // A thread to process in-flight command lists.
    std::thread             m_ProcessInFlightCommandListsThread;
    std::atomic_bool        m_bProcessInFlightCommandLists;
    std::mutex              m_ProcessInFlightCommandListsThreadMutex;
    std::condition_variable m_ProcessInFlightCommandListsThreadCV;
};
}

#endif //COMMANDQUEUE_H
