#pragma once

#include <memory>

#include "Core.h"
#include "Window.h"

namespace Enterprise
{
    class ENTERPRISE_API Application
    {
    public:
        Application();
        virtual ~Application();

        void Run();
    private:
        std::unique_ptr<Window> m_window;
    };

    Application* CreateApplication();

}
