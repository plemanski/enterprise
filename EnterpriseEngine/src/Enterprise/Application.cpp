#include "Application.h"

#include <algorithm>

#include "Core/Renderer.h"
// D3D12 extension library.

namespace Enterprise
{

Application::Application()
{
    m_window = std::unique_ptr<Window>(Window::Create());
    m_renderer = std::unique_ptr<Core::Graphics::Renderer>(Core::Graphics::Renderer::Create(m_window.get()));
}

Application::~Application()
{
}

void Application::Run()
{

    while (m_window->PumpEvents()) {};

}



}
