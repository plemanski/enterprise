//
// Created by Peter on 5/7/2025.
//

#include "W10Window.h"

namespace Enterprise {

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

W10Window::W10Window(const WindowProps &props)
{
    W10Window::Init(props);
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

std::unique_ptr<Window> Window::Create(const WindowProps &props)
{
    return std::make_unique<W10Window>(props);
}

void W10Window::Init(const WindowProps &props)
{

    RegisterWindowClass(nullptr, L"TestWindow");
    m_windowHandle =  CreateW10Window(L"TestWindow", nullptr, L"Test Window",props.Width, props.Height);
}

void W10Window::Shutdown()
{
}

void W10Window::RegisterWindowClass( HINSTANCE hInst, const wchar_t* windowClassName )
{
    // Register a window class for creating our render window with.
    m_wc = {};

    m_wc.cbSize = sizeof(WNDCLASSEX);
    m_wc.style = CS_HREDRAW | CS_VREDRAW;
    m_wc.lpfnWndProc = &WndProc;
    m_wc.cbClsExtra = 0;
    m_wc.cbWndExtra = 0;
    m_wc.hInstance = hInst;
    m_wc.hIcon = ::LoadIcon(hInst, NULL);
    m_wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    m_wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    m_wc.lpszMenuName = NULL;
    m_wc.lpszClassName = windowClassName;
    m_wc.hIconSm = ::LoadIcon(hInst, NULL);

    static ATOM atom = ::RegisterClassExW(&m_wc);
}

HWND W10Window::CreateW10Window(const wchar_t* windowClassName, HINSTANCE hInst, const wchar_t* windowTitle,
    uint32_t width, uint32_t height)
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
        NULL,
        windowClassName,
        windowTitle,
        WS_OVERLAPPEDWINDOW,
        windowX,
        windowY,
        windowWidth,
        windowHeight,
        NULL,
        NULL,
        hInst,
        nullptr
    );

    return hWnd;
}


LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

    switch (msg)
    {
        case VK_ESCAPE:
            ::PostQuitMessage(0);
            break;
    }
    return 0;
}


}
