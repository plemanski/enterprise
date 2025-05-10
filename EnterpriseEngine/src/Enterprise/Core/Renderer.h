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

#include "CommandQueue.h"
#include "../Window.h"
#include "../Events/ApplicationEvent.h"
#include "../Events/EventHandler.h"


namespace Enterprise::Core::Graphics {

class ENTERPRISE_API Renderer {
public:
    Renderer();
    ~Renderer() { Shutdown(); }
    static std::unique_ptr<Renderer> Create(const Window* window);


    void Initialize(const Window*);



    void Shutdown() const;

    void Flush(Microsoft::WRL::ComPtr<ID3D12CommandQueue>, Microsoft::WRL::ComPtr<ID3D12Fence>);

private:

    static constexpr uint8_t m_NumFrames = 3;
    uint32_t m_ClientWidth = 1280;
    uint32_t m_ClientHeight = 720;

    Microsoft::WRL::ComPtr<ID3D12Device2>               m_Device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>          m_CommandQueue;
    Microsoft::WRL::ComPtr<IDXGISwapChain4>             m_SwapChain;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_BackBuffers[m_NumFrames];
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_RTVDescriptorHeap;
    UINT m_RTVDescriptorSize = 0;
    UINT m_CurrentBackBufferIndex = 0;

    RECT m_WindowRect {};

    events::EventHandler<events::AppUpdateEvent> m_AppUpdateHandler;
    events::EventHandler<events::AppRenderEvent> m_AppRenderHandler;

private:
    void OnUpdateEvent();
    void OnRenderEvent();

    void DXUpdate();
    void DXRender();

    void Resize(uint32_t width, uint32_t height);

    void UpdateRenderTargetViews(Microsoft::WRL::ComPtr<ID3D12Device2>
                                 , Microsoft::WRL::ComPtr<IDXGISwapChain4>
                                 , Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>);

    Microsoft::WRL::ComPtr<IDXGIAdapter4> m_DxgiAdapter;
    Microsoft::WRL::ComPtr<ID3D12Device2> m_D3d12Device;

    std::shared_ptr<CommandQueue> m_DirectCommandQueue;
    bool m_VSync;
    bool m_TearingSupported;

};

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::exception();
    }
}

};

#endif //GRAPHICSCORE_H
