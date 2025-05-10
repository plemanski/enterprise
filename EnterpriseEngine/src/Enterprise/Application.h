#pragma once

#include <memory>

#include "Core.h"
#include "Window.h"
#include "Core/DescriptorHeap.h"
#include "Events/ApplicationEvent.h"
#include "Events/EventHandler.h"
#include "Core/Renderer.h"

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
        std::unique_ptr<Core::Graphics::Renderer>m_renderer;
        events::EventHandler<events::AppRenderEvent> m_AppRenderHandler;
        events::EventHandler<events::AppUpdateEvent> m_AppUpdateHandler;
    };

    Application* CreateApplication();

}
