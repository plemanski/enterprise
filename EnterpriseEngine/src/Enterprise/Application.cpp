#include "Application.h"

#include <algorithm>

#include "Core/Renderer.h"
// D3D12 extension library.

namespace Enterprise
{

Application::Application()
{
    m_window = std::unique_ptr(Window::Create());
    m_renderer = Core::Graphics::Renderer::Create(m_window.get());
}

Application::~Application()
{
    m_renderer->Shutdown();
}

void Application::Run()
{
    while (m_window->PumpEvents())
    {

    };
}



}
