//
// Created by Peter on 5/9/2025.
//

#ifndef GRAPHICSCORE_H
#define GRAPHICSCORE_H
#define NOMINMAX

#include <directx/d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <directx/d3dx12.h>
#include <exception>
#include <memory>
#include <wrl/client.h>
#include <algorithm>
#include <chrono>

#include "Camera.h"
#include "DescriptorAllocator.h"
#include "CommandQueue.h"
#include "DescriptorAllocation.h"
#include "HighResClock.h"
#include "Log.h"
#include "Mesh.h"
#include "RenderTarget.h"
#include "RootSignature.h"
#include "../Window.h"
#include "../Events/ApplicationEvent.h"
#include "../Events/EventHandler.h"


namespace Enterprise::Core::Graphics {
class DescriptorAllocator;
class Texture;
class ENTERPRISE_API Renderer {
public:
    Renderer(uint32_t width, uint32_t height);
    ~Renderer() { Shutdown(); }

    [[nodiscard]] static uint64_t GetFrameCount() { return ms_FrameCount; };
    static void IncrementFrameCount();

    const Camera *GetCamera() const { return &m_Camera; };

    static Renderer* Create(const Window* window);

    static Renderer* Get();

    void Initialize(const Window*);
    void Shutdown() const;

    [[nodiscard]] Microsoft::WRL::ComPtr<ID3D12Device2> GetDevice() const { return m_D3D12Device; };

    [[nodiscard]] Microsoft::WRL::ComPtr<ID3D12Resource> GetCurrentBackBuffer() const;

    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetView() const;
    [[nodiscard]] UINT GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE heapType ) const;

    DescriptorAllocation AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors = 1);
    /**
     * Release stale descriptors. This should only be called with a completed frame counter.
     */
    void ReleaseStaleDescriptors( uint64_t finishedFrame );
    [[nodiscard]] std::shared_ptr<CommandQueue> GetCommandQueue(D3D12_COMMAND_LIST_TYPE type ) const
    {
        std::shared_ptr<CommandQueue> commandQueue;
        switch (type)
        {
            case D3D12_COMMAND_LIST_TYPE_COPY:
                commandQueue = m_CopyCommandQueue;
                break;
            case D3D12_COMMAND_LIST_TYPE_COMPUTE:
                commandQueue = m_ComputeCommandQueue;
                break;
            case D3D12_COMMAND_LIST_TYPE_DIRECT:
                commandQueue = m_DirectCommandQueue;
                break;
            default:
                EE_CORE_WARN("Invalid command queue type requested");
                assert(false && "Invalid Command queue type");
        }
        return commandQueue;
    }
public:
    static constexpr uint32_t BUFFER_COUNT = 3;

private:
    void OnUpdateEvent(const events::AppUpdateEvent&);
    void OnRenderEvent(const events::AppRenderEvent&);
    UINT Present( const Texture& texture = Texture());
    void OnResizeEvent(const events::AppWindowResizeEvent&);

    void DXRender();

    void Resize(uint32_t width, uint32_t height);

    void Flush();

    void UpdateRenderTargetViews();

    void TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
        Microsoft::WRL::ComPtr<ID3D12Resource>resource,
        D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

    void ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
        D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor);

    void ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
        D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth = 1.0f);

    void UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
        ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource,
        size_t numElements, size_t elementSize, const void* bufferData,
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    void ResizeDepthBuffer(uint32_t width, uint32_t height);



    bool LoadContent();


private:
    static constexpr uint8_t                            ms_NumFrames = 3;
    Microsoft::WRL::ComPtr<IDXGIAdapter4>               m_DxgiAdapter;

    std::shared_ptr<CommandQueue>                       m_DirectCommandQueue;
    std::shared_ptr<CommandQueue>                       m_CopyCommandQueue;
    std::shared_ptr<CommandQueue>                       m_ComputeCommandQueue;
                                                        bool m_VSync;
                                                        bool m_TearingSupported;
    uint32_t                                            m_ClientWidth = 1280;
    uint32_t                                            m_ClientHeight = 720;
    uint64_t                                            m_FenceValues[BUFFER_COUNT] = {};
    uint64_t                                            m_FrameValues[BUFFER_COUNT] = {};

    Microsoft::WRL::ComPtr<ID3D12Device2>               m_D3D12Device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>          m_CommandQueue;
    Microsoft::WRL::ComPtr<IDXGISwapChain4>             m_SwapChain;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_BackBuffers[ms_NumFrames];
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_RTVDescriptorHeap;
    UINT                                                m_RTVDescriptorSize = 0;
    UINT                                                m_CurrentBackBufferIndex = 0;

    RECT                                                m_WindowRect {};

    Microsoft::WRL::ComPtr<ID3D12Resource>              m_VertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW                            m_VertexBufferView;

    Microsoft::WRL::ComPtr<ID3D12Resource>              m_IndexBuffer;
    D3D12_INDEX_BUFFER_VIEW                             m_IndexBufferView;

    Microsoft::WRL::ComPtr<ID3D12Resource>              m_DepthBuffer;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_DSVHeap;

    Microsoft::WRL::ComPtr<ID3D12RootSignature>         m_RootSignature;

    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_PipelineState;

    std::unique_ptr<DescriptorAllocator>                m_DescriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

    D3D12_VIEWPORT                                      m_Viewport;
    D3D12_RECT                                          m_ScissorRect;

    float                                               m_FoV;

    DirectX::XMMATRIX                                   m_ModelMatrix;
    DirectX::XMMATRIX                                   m_ViewMatrix;
    DirectX::XMMATRIX                                   m_ProjectionMatrix;

    bool                                                m_ContentLoaded;

    HighResClock                                        m_UpdateClock;
    HighResClock                                        m_RenderClock;

    events::EventHandler<events::AppUpdateEvent>        m_AppUpdateHandler;
    events::EventHandler<events::AppRenderEvent>        m_AppRenderHandler;
    events::EventHandler<events::AppWindowResizeEvent>  m_AppWindowResizeEventHandler;

    Texture                                             m_DefaultTexture;
    Texture                                             m_BackBufferTextures[BUFFER_COUNT];
    std::unique_ptr<Mesh>                               m_DemoCube;
    RenderTarget                                        m_RenderTarget;
    RootSignature                                       m_GraphicsRootSignature;
    static uint64_t                                     ms_FrameCount;
    Camera                                              m_Camera;
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
