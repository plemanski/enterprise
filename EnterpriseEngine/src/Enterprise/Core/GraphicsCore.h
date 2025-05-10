//
// Created by Peter on 5/9/2025.
//

#ifndef GRAPHICSCORE_H
#define GRAPHICSCORE_H
#define NOMINMAX

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <exception>
#include <memory>
#include <wrl/client.h>

#include "../Window.h"
#include "../Events/ApplicationEvent.h"
#include "../Events/EventHandler.h"


namespace Enterprise::Core::Graphics {
class CommandListManager;
class ContextManager {
public:
    ContextManager();
    virtual ~ContextManager() = default;

    void OnUpdateEvent();
    void OnRenderEvent();

private:
    events::EventHandler<events::AppRenderEvent> m_AppRenderHandler;
    events::EventHandler<events::AppUpdateEvent> m_AppUpdateHandler;

};

inline extern const uint8_t g_NumFrames = 3;
inline extern uint32_t g_ClientWidth = 1280;
inline extern uint32_t g_ClientHeight = 720;

inline extern Microsoft::WRL::ComPtr<ID3D12Device2>               g_Device = nullptr;
inline extern Microsoft::WRL::ComPtr<ID3D12CommandQueue>          g_CommandQueue = nullptr;
inline extern Microsoft::WRL::ComPtr<IDXGISwapChain4>             g_SwapChain = nullptr;
inline extern Microsoft::WRL::ComPtr<ID3D12Resource>              g_BackBuffers[g_NumFrames] = {};
inline extern Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   g_CommandList = nullptr;
inline extern Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      g_CommandAllocators[g_NumFrames] = {};
inline extern Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        g_RTVDescriptorHeap = nullptr;
inline extern UINT g_RTVDescriptorSize = 0;
inline extern UINT g_CurrentBackBufferIndex = 0;

inline extern RECT g_WindowRect {};

//Synchronization Objects
inline extern Microsoft::WRL::ComPtr<ID3D12Fence> g_Fence = nullptr;
inline extern uint64_t g_FenceValue = 0;
inline extern uint64_t g_FrameFenceValues[g_NumFrames] = {};
inline extern HANDLE g_FenceEvent = nullptr;

// By default, enable V-Sync.
// Can be toggled with the V key.
inline extern bool g_VSync = true;
inline extern bool g_TearingSupported = false;
// By default, use windowed mode.
// Can be toggled with the Alt+Enter or F11

extern CommandListManager   g_CommandListManager;
extern ContextManager       g_ContextManager;

void Initialize(const Window*);
void Shutdown();
extern void Flush(Microsoft::WRL::ComPtr<ID3D12CommandQueue>, Microsoft::WRL::ComPtr<ID3D12Fence>);

extern inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::exception();
    }
}

}

#endif //GRAPHICSCORE_H
