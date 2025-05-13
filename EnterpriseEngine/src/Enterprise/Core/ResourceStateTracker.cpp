//
// Created by Peter on 5/12/2025.
//

#include "ResourceStateTracker.h"

#include <cassert>

#include "CommandList.h"
#include "Resource.h"
#include "directx/d3dx12_barriers.h"

namespace Enterprise::Core::Graphics {

std::mutex ResourceStateTracker::ms_GlobalStateMutex;
bool ResourceStateTracker::ms_IsLocked = false;
ResourceStateTracker::ResourceStateMap ResourceStateTracker::ms_GlobalResourceState;

void ResourceStateTracker::ResourceBarrier(const D3D12_RESOURCE_BARRIER &barrier)
{
    if (barrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION )
    {
        const D3D12_RESOURCE_TRANSITION_BARRIER& transitionBarrier = barrier.Transition;

        // If there is an entry in FinalResourceState, the resource has been used and the state is known
        const auto iter = m_FinalResourceState.find( transitionBarrier.pResource );
        if ( iter != m_FinalResourceState.end() )
        {
            auto& resourceState = iter->second;

            if ( transitionBarrier.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES &&
                !resourceState.SubresourceState.empty() )
            {
                // Transition all subresources if they are different
                for ( auto subresourceState : resourceState.SubresourceState )
                {
                    if ( transitionBarrier.StateAfter != subresourceState.second )
                    {
                        D3D12_RESOURCE_BARRIER newBarrier = barrier;
                        newBarrier.Transition.Subresource = subresourceState.first;
                        newBarrier.Transition.StateBefore = subresourceState.second;
                        m_ResourceBarriers.push_back( newBarrier );
                    }
                }
            } else // Not pushing all subresources. So we push the last known state for the one subresource
            {
                auto finalState = resourceState.GetSubresourceState( transitionBarrier.Subresource );
                if ( transitionBarrier.StateAfter != finalState )
                {
                    D3D12_RESOURCE_BARRIER newBarrier = barrier;
                    newBarrier.Transition.StateBefore = finalState;
                    m_ResourceBarriers.push_back( newBarrier );
                }
            }
        } else // resource was not found in the final resource state. command list is using it for the first time
        {
            m_PendingResourceBarriers.push_back( barrier );
            m_FinalResourceState[ transitionBarrier.pResource ].SetSubResourceState( transitionBarrier.Subresource, transitionBarrier.StateAfter);
        }
    } else // non-transition barrier, push to the resource barriers array
    {
        m_ResourceBarriers.push_back(barrier);
    }
}

void ResourceStateTracker::TransitionResource(ID3D12Resource *resource, D3D12_RESOURCE_STATES stateAfter, UINT subResource)
{
   if ( resource )
   {
       ResourceBarrier( CD3DX12_RESOURCE_BARRIER::Transition( resource, D3D12_RESOURCE_STATE_COMMON, stateAfter, subResource ) );
   }
}

void ResourceStateTracker::TransitionResource(Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT subResource)
{
    TransitionResource( resource.GetD3D12Resource().Get(), stateAfter, subResource );
}

void ResourceStateTracker::UAVBarrier(Resource *resource)
{
    ID3D12Resource* pResource = resource != nullptr ? resource->GetD3D12Resource().Get() : nullptr;
    ResourceBarrier( CD3DX12_RESOURCE_BARRIER::UAV( pResource ));
}

void ResourceStateTracker::AliasBarrier( const Resource *resourceBefore, const Resource* resourceAfter)
{
    ID3D12Resource* pResourceBefore = resourceBefore != nullptr ? resourceBefore->GetD3D12Resource().Get() : nullptr;
    ID3D12Resource* pResourceAfter = resourceAfter != nullptr ? resourceAfter->GetD3D12Resource().Get() : nullptr;

    ResourceBarrier( CD3DX12_RESOURCE_BARRIER::Aliasing(pResourceBefore, pResourceAfter ) );
}

void ResourceStateTracker::FlushResourceBarriers( CommandList& commandList )
{
    UINT numBarriers = static_cast<UINT>( m_ResourceBarriers.size() );
    if ( numBarriers > 0 )
    {
        auto d3d12CommandList = commandList.GetGraphicsCommandList();
        d3d12CommandList->ResourceBarrier( numBarriers, m_ResourceBarriers.data() );
        m_ResourceBarriers.clear();
    }
}

uint32_t ResourceStateTracker::FlushPendingResourceBarriers(CommandList &commandList)
{
    assert( ms_IsLocked );

    ResourceBarriers resourceBarriers;
    resourceBarriers.reserve( m_PendingResourceBarriers.size() );
    for (auto pendingBarrier : m_PendingResourceBarriers )
    {
        if (pendingBarrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION )
        {
            auto pendingTransition = pendingBarrier.Transition;
            const auto& iter = ms_GlobalResourceState.find( pendingTransition.pResource );
            if ( iter != ms_GlobalResourceState.end() )
            {
                auto& resourceState = iter->second;
                if ( pendingTransition.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES &&
                    !resourceState.SubresourceState.empty() )
                {
                    // transition all subresources
                    for ( auto subresourceState : resourceState.SubresourceState )
                    {
                        if ( pendingTransition.StateAfter != subresourceState.second )
                        {
                            D3D12_RESOURCE_BARRIER newBarrier = pendingBarrier;
                            newBarrier.Transition.Subresource = subresourceState.first;
                            newBarrier.Transition.StateBefore = subresourceState.second;
                            resourceBarriers.push_back( newBarrier );
                        }
                    }
                } else
                {
                    auto globalState = ( iter->second ).GetSubresourceState( pendingTransition.Subresource );
                    if ( pendingTransition.StateAfter != globalState )
                    {
                        // Add cheeky transition to get the state sorted before executing command lists
                        pendingBarrier.Transition.StateBefore = globalState;
                        resourceBarriers.push_back( pendingBarrier );
                    }
                }
            }
        }
    }
    UINT numBarriers = static_cast<UINT>(resourceBarriers.size());
    if ( numBarriers > 0 )
    {
        auto d3d12CommandList = commandList.GetGraphicsCommandList();
        d3d12CommandList->ResourceBarrier( numBarriers, resourceBarriers );
    }

    m_PendingResourceBarriers.clear();
    return numBarriers;
}

void ResourceStateTracker::CommitFinalResourceStates()
{
    assert( ms_IsLocked );
    for ( const auto& resourceState : m_FinalResourceState )
    {
        ms_GlobalResourceState[resourceState.first] = resourceState.second;
    }

    m_FinalResourceState.clear();
}

void ResourceStateTracker::Reset()
{
    m_PendingResourceBarriers.clear();
    m_ResourceBarriers.clear();
    m_FinalResourceState.clear();
}

void ResourceStateTracker::Lock()
{
    ms_GlobalStateMutex.lock();
    ms_IsLocked = true;
}

void ResourceStateTracker::Unlock()
{
    ms_GlobalStateMutex.unlock();
    ms_IsLocked = false();
}

void ResourceStateTracker::RemoveGlobalResourceState(ID3D12Resource *resource)
{
    if ( resource != nullptr )
    {
        std::lock_guard<std::mutex> lock(ms_GlobalStateMutex);
        ms_GlobalResourceState.erase(resource);
    }
}

void ResourceStateTracker::AddGlobalResourceState(ID3D12Resource *resource, D3D12_RESOURCE_STATES state)
{
    if ( resource != nullptr)
    {
        std::lock_guard<std::mutex> lock( ms_GlobalStateMutex );
        ms_GlobalResourceState[resource].SetSubResourceState(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, state );
    }
}





}
