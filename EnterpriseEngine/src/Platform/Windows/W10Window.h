//
// Created by Peter on 5/7/2025.
//

#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <memory>
#include "Window.h"
#include "Events/EventManager.h"
#include "imgui_impl_win32.h"

namespace Enterprise {
class ENTERPRISE_API W10Window : public Window {
public:
    W10Window( const WindowProps &props );

    virtual ~W10Window() = default;

    void OnUpdate() override;

    unsigned int GetWidth() const override { return m_Data.Width; }
    unsigned int GetHeight() const override { return m_Data.Height; }

    inline void SetCallback( const EventCallbackFn &callback ) override { m_Data.Callback = callback; }

    void SetVSync( bool enabled ) override;

    bool IsVSync() const override;

    static std::unique_ptr<Window> Create( WindowProps &props );

    void *GetNativeWindow() const override { return m_windowHandle; }

    inline void SetWindowHandle( HWND hWnd ) { m_windowHandle = hWnd; }
    inline HWND GetWindowHandle() { return m_windowHandle; }

    LRESULT ProcessMessage( UINT msg, WPARAM wParam, LPARAM lParam );

    void ProcessRawInput() const;

    bool PumpEvents() const override;

public:
    static constexpr LONG MinWidth = 320;
    static constexpr LONG MinHeight = 240;

private:
    virtual void Init( const WindowProps &props );

    virtual void Shutdown();

    void RegisterWindowClass( HINSTANCE hInst, const wchar_t* windowClassName );

private:
    struct WindowData {
        std::string     Title;
        UINT            Width, Height;
        bool            VSync;
        bool            pauseAppWhenInactive;
        EventCallbackFn Callback;
    };

    UINT                m_CBSize;
    PRAWINPUT           m_RawInputCB;
    WindowData          m_Data;
    WNDCLASSEXW         m_wc{};
    HWND                m_windowHandle{};
    std::weak_ptr<RECT> m_windowRect{};
};
}
