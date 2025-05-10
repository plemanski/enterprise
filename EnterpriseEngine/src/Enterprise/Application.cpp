#include "Application.h"

#include <algorithm>

#include "Core/GraphicsCore.h"
// D3D12 extension library.

namespace Enterprise
{

Application::Application()
{
    m_window = std::unique_ptr<Window>(Window::Create());
    Core::Graphics::Initialize(m_window.get());
}

Application::~Application()
{
}

void Application::Run()
{

    while (m_window->PumpEvents()) {};
    ::CloseHandle(Core::Graphics::g_FenceEvent);

}



}
