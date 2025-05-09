#include "Application.h"

namespace Enterprise
{
    Application::Application()
    {
        m_window = std::unique_ptr<Window>(Window::Create());
    }

    Application::~Application()
    {
    }

    void Application::Run()
    {
        while (m_window->PumpEvents());
    }
}
