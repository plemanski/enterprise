//
// Created by Peter on 5/7/2025.
//

#include <algorithm>

#include "W10Window.h"

#include "Log.h"
#include "Core/Renderer.h"
#include "Events/ApplicationEvent.h"
#include "Events/EventManager.h"


namespace Enterprise {

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

W10Window::W10Window(const WindowProps &props)
{
    m_Data = {};
    m_Data.Title = props.Title;
    m_Data.Width = props.Width;
    m_Data.Height = props.Height;
    m_Data.pauseAppWhenInactive = true;
}

void W10Window::OnUpdate()
{
}

void W10Window::SetVSync(bool enabled)
{
}

bool W10Window::IsVSync() const
{
    return false;
}

HWND CreateW10Window(const wchar_t* windowClassName, HINSTANCE hInst, const wchar_t* windowTitle,
                                uint32_t width, uint32_t height, LPVOID lParam)
{
    int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

    RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    ::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    int windowX = std::max<int>(0, (screenWidth - windowWidth) / 2);
    int windowY = std::max<int>(0, (screenHeight - windowHeight) / 2);
    HWND hWnd = ::CreateWindowExW(
        WS_EX_OVERLAPPEDWINDOW,
        windowClassName,
        windowTitle,
        WS_OVERLAPPEDWINDOW,
        windowX,
        windowY,
        windowWidth,
        windowHeight,
        HWND_DESKTOP,
        NULL,
        hInst,
        lParam
    );
    if (!hWnd)
    {
        EE_CORE_ERROR(GetLastError());
    }

    return hWnd;
}

std::unique_ptr<Window> Window::Create(const WindowProps& props)
{
    auto window = std::make_unique<W10Window>(props);

    // Initialize Window class
    WNDCLASSEXW wc = {};

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = &WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hIcon = ::LoadIcon(GetModuleHandle(nullptr), NULL);
    wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = L"TestWindow";
    wc.hIconSm = nullptr;

    static auto wndClassAtom = ::RegisterClassExW(&wc);
    auto hWnd =  CreateW10Window(L"TestWindow", GetModuleHandle(nullptr), L"Test Window",props.Width, props.Height, window.get());
    if (hWnd == nullptr)
    {
        EE_CORE_ERROR("Window handle is null.");
    }
    window->SetWindowHandle(hWnd);
    return window;
}

void W10Window::Init(const WindowProps &props)
{
    ::ShowWindow(m_windowHandle, SW_SHOWDEFAULT);
    ::UpdateWindow(m_windowHandle);
}

void W10Window::Shutdown()
{
}

void W10Window::RegisterWindowClass( HINSTANCE hInst, const wchar_t* windowClassName )
{
    // Register a window class for creating our render window with.

}


// Some events borrowed from Mike Marcin:
// https://mikemarcin.com/posts/letsmake_engine_02_windowhttps://mikemarcin.com/posts/letsmake_engine_02_window//
LRESULT W10Window::ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        // Credit: Mike Marcin
        case WM_ACTIVATEAPP:
        {
            if (m_Data.pauseAppWhenInactive)
            {
                const bool isBecomingActive = (wParam == TRUE);
                const auto priorityClass = isBecomingActive ? NORMAL_PRIORITY_CLASS : IDLE_PRIORITY_CLASS;
                SetPriorityClass(GetCurrentProcess(), priorityClass);
            }
            break;
        }
        case WM_PAINT:
        {
            //Core::Graphics::Renderer::IncrementFrameCount();

            //events::TriggerEvent(events::AppUpdateEvent(0.0f, 0.0f, Core::Graphics::Renderer::GetFrameCount()));
            //events::TriggerEvent(events::AppRenderEvent(0.0f, 0.0f, Core::Graphics::Renderer::GetFrameCount()));
            break;
        }
        // Credit: Mike Marcin
        case WM_SIZING:
        {
            // maintain aspec ratio when resizing
            auto& rect = *reinterpret_cast<RECT*>(lParam);
            const float aspectRatio = float(m_Data.Width) / m_Data.Height;
            const DWORD style = GetWindowLong(m_windowHandle, GWL_STYLE);
            const DWORD exStyle = GetWindowLong(m_windowHandle, GWL_EXSTYLE);

            RECT emptyRect{};
            AdjustWindowRectEx(&emptyRect, style, false, exStyle);
            rect.left -= emptyRect.left;
            rect.right -= emptyRect.right;
            rect.top -= emptyRect.top;
            rect.bottom -= emptyRect.bottom;

            auto newWidth = std::max(rect.right - rect.left, MinWidth);
            auto newHeight = std::max(rect.bottom - rect.top, MinHeight);

            switch (wParam) {
                case WMSZ_LEFT:
                    newHeight = static_cast<LONG>(newWidth / aspectRatio);
                    rect.left = rect.right - newWidth;
                    rect.bottom = rect.top + newHeight;
                    break;
                case WMSZ_RIGHT:
                    newHeight = static_cast<LONG>(newWidth / aspectRatio);
                    rect.right = rect.left + newWidth;
                    rect.bottom = rect.top + newHeight;
                    break;
                case WMSZ_TOP:
                case WMSZ_TOPRIGHT:
                    newWidth = static_cast<LONG>(newHeight * aspectRatio);
                    rect.right = rect.left + newWidth;
                    rect.top = rect.bottom - newHeight;
                    break;
                case WMSZ_BOTTOM:
                case WMSZ_BOTTOMRIGHT:
                    newWidth = static_cast<LONG>(newHeight * aspectRatio);
                    rect.right = rect.left + newWidth;
                    rect.bottom = rect.top + newHeight;
                    break;
                case WMSZ_TOPLEFT:
                    newWidth = static_cast<LONG>(newHeight * aspectRatio);
                    rect.left = rect.right - newWidth;
                    rect.top = rect.bottom - newHeight;
                    break;
                case WMSZ_BOTTOMLEFT:
                    newWidth = static_cast<LONG>(newHeight * aspectRatio);
                    rect.left = rect.right - newWidth;
                    rect.bottom = rect.top + newHeight;
                    break;
            }
            events::TriggerEvent(events::AppWindowResizeEvent(newWidth, newHeight));
            AdjustWindowRectEx(&rect, style, false, exStyle);
            return TRUE;
        }
        case WM_NCDESTROY:
        {
            PostQuitMessage(0);
            m_windowHandle = nullptr;
            SetWindowLongPtr(m_windowHandle, GWLP_USERDATA, LONG_PTR(0));
            break;
        }
        default:
            return DefWindowProc(m_windowHandle, msg, wParam, lParam);
    }
    return DefWindowProc(m_windowHandle, msg, wParam, lParam);
}

bool W10Window::PumpEvents() const
{
    MSG msg{};
    while (WM_QUIT != msg.message)
    {
        if(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else
        {
            events::TriggerEvent(events::AppUpdateEvent(0.0f, 0.0f, Core::Graphics::Renderer::GetFrameCount()));
            events::TriggerEvent(events::AppRenderEvent(0.0f, 0.0f, Core::Graphics::Renderer::GetFrameCount()));
        }

    }
    return true;
}

// Set GWLP_USERDATA as a pointer to W10Window on WM_NCCREATE, otherwise get pointer to window and call ProcessMessage
LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    W10Window* self = nullptr;

    if (msg != WM_NCCREATE)
    {
        self = reinterpret_cast<W10Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    } else
    {
        CREATESTRUCT *createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
        self = reinterpret_cast<W10Window*>(createStruct->lpCreateParams);
        self->SetWindowHandle(hWnd);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }

    if (self != nullptr)
    {
        return self->ProcessMessage(msg, wParam, lParam);
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);

}


}
