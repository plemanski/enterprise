//
// Created by Peter on 5/7/2025.
//

#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <memory>
#include <minwindef.h>
#include "Window.h"
#include "imgui_impl_win32.h"

namespace Enterprise {

class ENTERPRISE_API W10Window : public Window {
public:
    W10Window(const WindowProps &props);
    virtual ~W10Window() = default;

    void OnUpdate() override;

    unsigned int GetWidth() const override { return m_Data.Width; }
    unsigned int GetHeight() const override { return m_Data.Height; }

    inline void SetCallback(const EventCallbackFn& callback) override { m_Data.Callback = callback; }

    void SetVSync(bool enabled) override;
    bool IsVSync() const override;


private:
    HWND CreateW10Window(const wchar_t* windowClassName, HINSTANCE hInst, const wchar_t* windowTitle,
        uint32_t width, uint32_t height);

    virtual void Init(const WindowProps& props);
    virtual void Shutdown();

    void RegisterWindowClass(HINSTANCE hInst, const wchar_t *windowClassName);

private:
    struct WindowData {
        std::string Title;
        UINT Width, Height;
        bool VSync;
        EventCallbackFn Callback;
    };

    WindowData m_Data;

    WNDCLASSEXW m_wc{};
    HWND m_windowHandle{};
};

}

