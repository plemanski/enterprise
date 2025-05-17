//
// Created by Peter on 5/9/2025.
//

#include "Renderer.h"

#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <memory>
#include <cassert>
#include <chrono>

#include "CommandList.h"
#include "CommandQueue.h"
#include "DescriptorAllocator.h"
#include "directx/d3dx12_barriers.h"
#include "directx/d3dx12_root_signature.h"
#include "Resource.h"
#include "../Window.h"
#include "Events/EventManager.h"
#include "spdlog/fmt/bundled/base.h"


namespace Enterprise::Core::Graphics {
using namespace Microsoft::WRL;

using namespace DirectX;

struct Transforms {
    XMMATRIX ModelMatrix;
    XMMATRIX ModelViewMatrix;
    XMMATRIX InverseTransposeMatrix;
    XMMATRIX ModelViewProjectionMatrix;
};

void XM_CALLCONV ComputeMatrices( FXMMATRIX model, CXMMATRIX view, CXMMATRIX viewProjection, Transforms &transforms )
{
    transforms.ModelMatrix = model;
    transforms.ModelViewMatrix = model * view;
    transforms.InverseTransposeMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, transforms.ModelViewMatrix));
    transforms.ModelViewProjectionMatrix = model * viewProjection;
}

namespace VertexPositionNormalTexture {
}

uint64_t Renderer::ms_FrameCount = 0;

Renderer* gs_pRenderer = nullptr;

XMMATRIX XM_CALLCONV LookAtMatrix( FXMVECTOR Position, FXMVECTOR Direction, FXMVECTOR Up )
{
    assert(!XMVector3Equal( Direction, XMVectorZero() ));
    assert(!XMVector3IsInfinite( Direction ));
    assert(!XMVector3Equal( Up, XMVectorZero() ));
    assert(!XMVector3IsInfinite( Up ));

    XMVECTOR R2 = XMVector3Normalize(Direction);

    XMVECTOR R0 = XMVector3Cross(Up, R2);
    R0 = XMVector3Normalize(R0);

    XMVECTOR R1 = XMVector3Cross(R2, R0);

    XMMATRIX M(R0, R1, R2, Position);

    return M;
}

void Renderer::UpdateBufferResource( ComPtr<ID3D12GraphicsCommandList2> commandList,
                                     ID3D12Resource** pDestinationResource,
                                     ID3D12Resource** pIntermediateResource,
                                     size_t numElements, size_t elementSize, const void* bufferData,
                                     D3D12_RESOURCE_FLAGS flags )
{
    size_t bufferSize = numElements * elementSize;

    ThrowIfFailed(m_D3D12Device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags),
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(pDestinationResource)));

    if (bufferData)
    {
        ThrowIfFailed(m_D3D12Device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(pIntermediateResource)));

        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = bufferData;
        subresourceData.RowPitch = bufferSize;
        subresourceData.SlicePitch = subresourceData.RowPitch;

        UpdateSubresources(commandList.Get(),
                           *pDestinationResource, *pIntermediateResource,
                           0, 0, 1, &subresourceData);
    }
}

bool Renderer::LoadContent()
{
    auto commandList = m_CopyCommandQueue->GetCommandList();
    m_DemoCube = Mesh::CreateDemoCube(*commandList, 1);

    commandList->LoadTextureFromFile(m_DefaultTexture, L"C:/dev/Enterprise/EnterpriseEngine/resources/assets/textures/DefaultWhite.bmp", false);
    //D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    //dsvHeapDesc.NumDescriptors = 1;
    //dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    //dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    //ThrowIfFailed(m_D3D12Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DSVHeap)));
    m_CopyCommandQueue->ExecuteCommandList(commandList);

    DXGI_FORMAT sdrFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT depthFormat = DXGI_FORMAT_D32_FLOAT;

    auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(sdrFormat, m_ClientWidth, m_ClientHeight);
    colorDesc.MipLevels = 1;
    colorDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE colorClearValue;
    colorClearValue.Format = colorDesc.Format;
    colorClearValue.Color[0] = 0.4f;
    colorClearValue.Color[1] = 0.6f;
    colorClearValue.Color[2] = 0.9f;
    colorClearValue.Color[3] = 1.0f;

    Texture sdrTexture = Texture(colorDesc, &colorClearValue, L"SDR-Render Target");

    auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthFormat, m_ClientWidth, m_ClientHeight);
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    depthDesc.MipLevels = 1;

    D3D12_CLEAR_VALUE depthClearValue;
    depthClearValue.Format = depthDesc.Format;
    depthClearValue.DepthStencil = {1.0f, 0};

    Texture depthTexture = Texture(depthDesc, &depthClearValue, L"Depth-Render Target");

    m_RenderTarget.AttachTexture(AttachmentPoint::Color0, sdrTexture);
    m_RenderTarget.AttachTexture(AttachmentPoint::DepthStencil, depthTexture);

    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"VertexShader.cso", &vertexShaderBlob));

    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"PixelShader.cso", &pixelShaderBlob));

    //TODO change shader to store vertex buffer data in a StructeredBuffer in vertex shader
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
        {
            "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
        {
            "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
    };

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(m_D3D12Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS ;

    CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);

    CD3DX12_ROOT_PARAMETER1 rootParameters[2];
    rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &linearRepeatSampler, rootSignatureFlags);

    //Serialize root signature
    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    m_GraphicsRootSignature.SetRootSignatureDesc(rootSignatureDesc.Desc_1_1, featureData.HighestVersion);
    //ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc,
    //    featureData.HighestVersion, &rootSignatureBlob, &errorBlob));

    //// Create root signature
    //ThrowIfFailed(m_Device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
    //    rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));

    struct PipelineStateStream {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE        pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT          InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY    PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS                    VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS                    PS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT  DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
        CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC           SampleDesc;
    } pipelineStateStream;

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = sdrFormat;

    pipelineStateStream.pRootSignature = m_GraphicsRootSignature.GetRootSignature().Get();
    pipelineStateStream.InputLayout = {inputLayout, _countof(inputLayout)};
    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    pipelineStateStream.DSVFormat = depthFormat;
    pipelineStateStream.RTVFormats = rtvFormats;

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };
    ThrowIfFailed(m_D3D12Device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)));

    //auto fenceValue = m_CopyCommandQueue->ExecuteCommandList(commandList);
    //m_CopyCommandQueue->WaitForFenceValue(fenceValue);
    m_CopyCommandQueue->Flush();
    m_ContentLoaded = true;

    //ResizeDepthBuffer(m_ClientWidth, m_ClientHeight);
    return true;
}

void Renderer::ResizeDepthBuffer( uint32_t width, uint32_t height )
{
    if (m_ContentLoaded)
    {
        Flush();

        width = std::max(static_cast<uint32_t>(1), width);
        height = std::max(static_cast<uint32_t>(1), height);

        D3D12_CLEAR_VALUE optimizedClearValue = {};
        optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        optimizedClearValue.DepthStencil = {1.0f, 0};

        ThrowIfFailed(m_D3D12Device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height,
                                          1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &optimizedClearValue,
            IID_PPV_ARGS(&m_DepthBuffer)
        ));

        D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
        dsv.Format = DXGI_FORMAT_D32_FLOAT;
        dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsv.Texture2D.MipSlice = 0;
        dsv.Flags = D3D12_DSV_FLAG_NONE;

        m_D3D12Device->CreateDepthStencilView(m_DepthBuffer.Get(), &dsv,
                                              m_DSVHeap->GetCPUDescriptorHandleForHeapStart());
    }
}

ComPtr<ID3D12Resource> Renderer::GetCurrentBackBuffer() const
{
    return m_BackBuffers[m_CurrentBackBufferIndex];
}

D3D12_CPU_DESCRIPTOR_HANDLE Renderer::GetCurrentRenderTargetView() const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                                         m_CurrentBackBufferIndex, m_RTVDescriptorSize);
}

UINT Renderer::GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE heapType ) const
{
    return m_D3D12Device->GetDescriptorHandleIncrementSize(heapType);
}

DescriptorAllocation Renderer::AllocateDescriptors( D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors )
{
    return m_DescriptorAllocators[type]->Allocate(numDescriptors);
}

void Renderer::ReleaseStaleDescriptors( uint64_t finishedFrame )
{
    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        m_DescriptorAllocators[i]->ReleaseStaleDescriptors(finishedFrame);
    }
}


void EnableDebugLayer()
{
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugInterface;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
    debugInterface->EnableDebugLayer();
#endif
}

ComPtr<IDXGIAdapter4> GetAdapter( bool useWarp )
{
    ComPtr<IDXGIFactory6> dxgiFactory6;
    UINT                  createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory6)));

    ComPtr<IDXGIAdapter1> dxgiAdapter1;
    ComPtr<IDXGIAdapter4> dxgiAdapter4;

    if (useWarp)
    {
        ComPtr<IDXGIFactory4> dxgiFactory4;
        ThrowIfFailed(dxgiFactory4->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
        ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
    } else
    {
        for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory6->EnumAdapterByGpuPreference(
                                        adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                        IID_PPV_ARGS(&dxgiAdapter1)); ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
            dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

            if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0
                && SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(),
                    D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
            {
                ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
            }
        }
    }
    return dxgiAdapter4;
}

ComPtr<ID3D12Device2> CreateDevice( ComPtr<IDXGIAdapter4> adapter )
{
    ComPtr<ID3D12Device2> d3d12Device2;
    ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2)));
    // Enable debug messages in debug mode.
#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> pInfoQueue;
    if (SUCCEEDED(d3d12Device2.As(&pInfoQueue)))
    {
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
        // Suppress whole categories of messages
        //D3D12_MESSAGE_CATEGORY Categories[] = {};

        // Suppress messages based on their severity level
        D3D12_MESSAGE_SEVERITY Severities[] =
        {
            D3D12_MESSAGE_SEVERITY_INFO
        };

        // Suppress individual messages by their ID
        D3D12_MESSAGE_ID DenyIds[] = {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
            // I'm really not sure how to avoid this message.
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            // This warning occurs when using capture frame while graphics debugging.
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
            // This warning occurs when using capture frame while graphics debugging.
        };

        D3D12_INFO_QUEUE_FILTER NewFilter = {};
        //NewFilter.DenyList.NumCategories = _countof(Categories);
        //NewFilter.DenyList.pCategoryList = Categories;
        NewFilter.DenyList.NumSeverities = _countof(Severities);
        NewFilter.DenyList.pSeverityList = Severities;
        NewFilter.DenyList.NumIDs = _countof(DenyIds);
        NewFilter.DenyList.pIDList = DenyIds;

        ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
    }
#endif

    return d3d12Device2;
}


bool CheckTearingSupport()
{
    // TODO: Impl
    bool allowTearing = FALSE;
    return allowTearing;
}

ComPtr<IDXGISwapChain4> CreateSwapChain( HWND                       hWnd,
                                         ComPtr<ID3D12CommandQueue> commandQueue,
                                         uint32_t                   width, uint32_t height, uint32_t bufferCount )
{
    ComPtr<IDXGISwapChain4> dxgiSwapChain4;
    ComPtr<IDXGIFactory4>   dxgiFactory4;
    UINT                    createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

    // Swap Chain Creation Params
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc = {1, 0};
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = bufferCount;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    // Recommended to enable tearing support if available
    swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    ComPtr<IDXGISwapChain1> swapChain1;
    ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
        commandQueue.Get(),
        hWnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain1));

    // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
    // will be handled manually.
    ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain1.As(&dxgiSwapChain4));

    return dxgiSwapChain4;
}

ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap( ComPtr<ID3D12Device2>      device,
                                                   D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors )
{
    ComPtr<ID3D12DescriptorHeap> descriptorHeap;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};

    desc.NumDescriptors = numDescriptors;
    desc.Type = type;

    ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

    return descriptorHeap;
}

void Renderer::UpdateRenderTargetViews()
{
    for (uint8_t i = 0; i < ms_NumFrames; ++i)
    {
        ComPtr<ID3D12Resource> backBuffer;
        ThrowIfFailed(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

        ResourceStateTracker::AddGlobalResourceState(backBuffer.Get(), D3D12_RESOURCE_STATE_COMMON);

        m_BackBufferTextures[i] = Texture(backBuffer, L"Backbuffer[" + std::to_wstring(i) + L"]");
        m_BackBufferTextures[i].CreateViews();
    }
}

HANDLE CreateEventHandle()
{
    HANDLE fenceEvent;

    fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    assert(fenceEvent && "Failed to create fence event.");

    return fenceEvent;
}

void Renderer::OnUpdateEvent( const events::AppUpdateEvent &event )
{
    m_UpdateClock.Tick();
    static uint64_t frameCounter = 0;
    static double   elapsedSeconds = 0.0;

    elapsedSeconds += m_UpdateClock.GetDeltaSeconds();
    frameCounter++;

    if (elapsedSeconds > 1.0)
    {
        auto fps = frameCounter / elapsedSeconds;

        char buffer[512];
        sprintf_s(buffer, 500, "FPS: %f/n", fps);
        OutputDebugString(buffer);

        frameCounter = 0;
        elapsedSeconds = 0.0;
    }

    float          angle = static_cast<float>(event.TotalTime * 90.0);
    //const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
    //m_ModelMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

    //const XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
    //const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
    //const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);

    //m_ViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

    //float aspectRatio = m_ClientWidth / static_cast<float>(m_ClientHeight);

    //m_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_FoV), aspectRatio, 0.5f, 100.0f);
}

void Renderer::TransitionResource( ComPtr<ID3D12GraphicsCommandList2> commandList, ComPtr<ID3D12Resource> resource,
                                   D3D12_RESOURCE_STATES              beforeState, D3D12_RESOURCE_STATES  afterState )
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        resource.Get(),
        beforeState, afterState);

    commandList->ResourceBarrier(1, &barrier);
}

void Renderer::ClearRTV( ComPtr<ID3D12GraphicsCommandList2> commandList,
                         D3D12_CPU_DESCRIPTOR_HANDLE        rtv, FLOAT* clearColor )
{
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void Renderer::ClearDepth( ComPtr<ID3D12GraphicsCommandList2> commandList,
                           D3D12_CPU_DESCRIPTOR_HANDLE        dsv, FLOAT depth )
{
    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void Renderer::Resize( uint32_t width, uint32_t height )
{
    if (m_ClientWidth != width || m_ClientHeight != height)
    {
        m_ClientHeight = std::max(1u, width);
        m_ClientHeight = std::max(1u, height);

        // Flush the backbuffer to make sure the swap chain's back buffer
        // are not being referenced by an in-flight command list.
        //Flush(m_CommandQueue, m_Fence, m_FenceValue, m_FenceEvent);
        m_DirectCommandQueue->Flush();

        for (uint8_t i = 0; i < ms_NumFrames; ++i)
        {
            // Release any references to the back buffers before resizing swap chain
            m_BackBuffers[i].Reset();
            //m_FrameFenceValues[i] = m_FrameFenceValues[m_CurrentBackBufferIndex];
        }

        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        ThrowIfFailed(m_SwapChain->GetDesc(&swapChainDesc));
        ThrowIfFailed(m_SwapChain->ResizeBuffers(ms_NumFrames, m_ClientWidth, m_ClientHeight,
                                                 swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

        m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

        UpdateRenderTargetViews();
    }
}

void Renderer::Flush()
{
    m_CopyCommandQueue->Flush();
    m_DirectCommandQueue->Flush();
}


void Renderer::Initialize( const Window* window )
{
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    EnableDebugLayer();

    HWND hWnd = static_cast<HWND>(window->GetNativeWindow());

    ::GetWindowRect(hWnd, &m_WindowRect);
    float width = window->GetWidth();
    float height = window->GetHeight();
    float aspect =  width / height;

    m_Camera.SetProjection(45.0, aspect, 0.01f, 100.0f);
    ComPtr<IDXGIAdapter4> dxgiAdapter4 = GetAdapter(FALSE);

    if (dxgiAdapter4)
        m_D3D12Device = CreateDevice(dxgiAdapter4);

    if (m_D3D12Device)
    {
        m_DirectCommandQueue = std::make_shared<CommandQueue>(D3D12_COMMAND_LIST_TYPE_DIRECT);
        m_CopyCommandQueue = std::make_shared<CommandQueue>(D3D12_COMMAND_LIST_TYPE_COPY);
        // m_TearingSupported = CheckTearingSupport();
    }

    m_SwapChain = CreateSwapChain(hWnd, m_DirectCommandQueue->GetD3D12CommandQueue(), window->GetWidth(),
                                  window->GetHeight(), ms_NumFrames);

    m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();


    for (UINT8 i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        m_DescriptorAllocators[i] = std::make_unique<DescriptorAllocator>(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
    }

    UpdateRenderTargetViews();
    ::ShowWindow(hWnd, SW_SHOW);
    ms_FrameCount = 0;
}

void Renderer::Shutdown() const
{
    m_DirectCommandQueue->Flush();
}


void Renderer::OnRenderEvent( const events::AppRenderEvent &event )
{
    if (!m_ContentLoaded)
    {
        return;
    }
    auto commandList = m_DirectCommandQueue->GetCommandList();

    // Clear render targets.
    {
        FLOAT clearColor[] = {0.4f, 0.6f, 0.9f, 1.0f};
        //TransitionResource(commandList, backBuffer,
        //    D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ClearTexture(m_RenderTarget.GetTexture(AttachmentPoint::Color0), clearColor);
        commandList->ClearDepthStencilTexture(m_RenderTarget.GetTexture(AttachmentPoint::DepthStencil),
                                              D3D12_CLEAR_FLAG_DEPTH);
    }

    commandList->SetRenderTarget(m_RenderTarget);
    commandList->SetViewport(m_RenderTarget.GetViewport());
    commandList->SetScissorRect(m_ScissorRect);

    VertexBuffer vertexBuffer;
    IndexBuffer  indexBuffer;
    //// Set Input Assembler state
    //commandList->IASetIndexBuffer(&m_IndexBufferView);
    commandList->SetPipelineState(m_PipelineState);
    commandList->SetGraphicsRootSignature(m_GraphicsRootSignature);
    //// Set Rasterizer state
    //commandList->RSSetViewports(1, &m_Viewport);
    //commandList->RSSetScissorRects(1, &m_ScissorRect);

    //// Bind render targets to the Output Merger
    //commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

    //// TODO: Update Transforms and bind to root signature
    XMMATRIX translationMatrix    = XMMatrixTranslation( 4.0f, 2.0f, 4.0f );
    XMMATRIX rotationMatrix       = XMMatrixIdentity();
    XMMATRIX scaleMatrix          = XMMatrixScaling( 4.0f, 4.0f, 4.0f );
    XMMATRIX worldMatrix         = scaleMatrix * rotationMatrix * translationMatrix;
    XMMATRIX viewMatrix           = m_Camera.GetViewMatrix();
    m_ProjectionMatrix = viewMatrix * m_Camera.GetProjectionMatrix();

    Transforms transform;
    // TODO: Implement Camera
    ComputeMatrices(worldMatrix, viewMatrix, m_ProjectionMatrix, transform);
    commandList->SetGraphicsDynamicConstantBuffer(0, sizeof(Transforms), &transform);
    commandList->SetShaderResourceView(1, 0,
                                       m_DefaultTexture,
                                       D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    //commandList->DrawIndexedInstanced(_countof(g_Indicies), 1, 0, 0, 0);
    m_DemoCube->Draw(*commandList);
    m_DirectCommandQueue->ExecuteCommandList(commandList);
    auto tex = m_RenderTarget.GetTexture(AttachmentPoint::Color0);

    Present(m_RenderTarget.GetTexture(AttachmentPoint::Color0));
    //Present
    //{
    //    TransitionResource(commandList, backBuffer,
    //                       D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    //    m_FenceValues[m_CurrentBackBufferIndex] = m_DirectCommandQueue->ExecuteCommandList(commandList);

    //    auto currentBackBufferIndex = Present();

    //    m_DirectCommandQueue->WaitForFenceValue(m_FenceValues[currentBackBufferIndex]);
    //}
}

UINT Renderer::Present( const Texture &texture )
{
    auto &backBuffer = m_BackBufferTextures[m_CurrentBackBufferIndex];
    auto  commandList = m_DirectCommandQueue->GetCommandList();

    if (texture.IsValid())
    {
        if (texture.GetD3D12ResourceDesc().SampleDesc.Count > 1)
        {
            commandList->ResolveSubresource(backBuffer, texture);
        } else
        {
            commandList->CopyResource(backBuffer, texture);
        }
    }

    RenderTarget renderTarget;
    renderTarget.AttachTexture(AttachmentPoint::Color0, backBuffer);

    commandList->TransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_PRESENT);
    m_DirectCommandQueue->ExecuteCommandList(commandList);

    UINT syncInterval = m_VSync ? 1 : 0;
    UINT presentFlags = m_TearingSupported && !m_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;

    ThrowIfFailed(m_SwapChain->Present(syncInterval, presentFlags));

    m_FenceValues[m_CurrentBackBufferIndex] = m_DirectCommandQueue->Signal();
    m_FrameValues[m_CurrentBackBufferIndex] = GetFrameCount();

    m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

    m_DirectCommandQueue->WaitForFenceValue(m_FenceValues[m_CurrentBackBufferIndex]);

    ReleaseStaleDescriptors(m_FrameValues[m_CurrentBackBufferIndex]);

    return m_CurrentBackBufferIndex;
}


void Renderer::OnResizeEvent( const events::AppWindowResizeEvent &event )
{
    auto width = event.Width;
    auto height = event.Height;

    if (width != m_ClientWidth || height != m_ClientHeight)
    {
        m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f,
                                      static_cast<float>(width), static_cast<float>(height));

        ResizeDepthBuffer(width, height);
    }
}

void XM_CALLCONV ComputeMatrices( FXMMATRIX model, CXMMATRIX view, CXMMATRIX viewProjection, Mat &mat )
{
    mat.ModelMatrix = model;
    mat.ModelViewMatrix = model * view;
    mat.InverseTransposeModelViewMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, mat.ModelViewMatrix));
    mat.ModelViewProjectionMatrix = model * viewProjection;
}

Renderer::Renderer( uint32_t width, uint32_t height )
    : m_ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
      , m_Viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)))
      , m_FoV(45.0)
      , m_ContentLoaded(false)
      , m_AppUpdateHandler([this]( const events::AppUpdateEvent &e ) { OnUpdateEvent(e); })
      , m_AppRenderHandler([this]( const events::AppRenderEvent &e ) { OnRenderEvent(e); })
      , m_AppWindowResizeEventHandler([this]( const events::AppWindowResizeEvent &e ) { OnResizeEvent(e); })
      , m_Camera()
{
    events::Subscribe<events::AppRenderEvent>(m_AppRenderHandler);
    events::Subscribe<events::AppUpdateEvent>(m_AppUpdateHandler);
}

void Renderer::IncrementFrameCount()
{
    ++ms_FrameCount;
}

Renderer *Renderer::Create( const Window* window )
{
    gs_pRenderer = new Renderer(window->GetWidth(), window->GetHeight());
    gs_pRenderer->Initialize(window);
    auto isLoaded = gs_pRenderer->LoadContent();
    return gs_pRenderer;
}

Renderer *Renderer::Get()
{
    return gs_pRenderer;
}
}
