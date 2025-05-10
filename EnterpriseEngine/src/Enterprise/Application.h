#pragma once

#include <memory>

#include "Core.h"
#include "Window.h"
#include "Events/ApplicationEvent.h"
#include "Events/EventHandler.h"

namespace Enterprise
{
    class ENTERPRISE_API Application
    {
    public:
        void OnRenderEvent();
        void OnUpdateEvent();

        Application();
        virtual ~Application();

        void Run();
    private:
        std::unique_ptr<Window> m_window;
        events::EventHandler<events::AppRenderEvent> m_AppRenderHandler;
        events::EventHandler<events::AppUpdateEvent> m_AppUpdateHandler;
    };

    Application* CreateApplication();

}
