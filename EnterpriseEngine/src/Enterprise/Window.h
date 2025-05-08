//
// Created by Peter on 5/7/2025.
//

#pragma once
#include <functional>
#include <string>

#include "Events/event.h"
#include "Core.h"

namespace Enterprise {
    struct WindowProps
    {
        std::string Title;
        unsigned int Width;
        unsigned int Height;

        explicit WindowProps(const std::string& title = "Enterprise Engine",
                             unsigned int width = 1280,
                             unsigned int height = 720)
                        : Title(title), Width(width), Height(height)
        {
        }
    };

    class ENTERPRISE_API Window {
    public:
        virtual ~Window() = default;
        using EventCallbackFn = std::function<void(events::Event&)>;

        virtual void OnUpdate() = 0;

        virtual unsigned int GetWidth() const = 0;
        virtual unsigned int GetHeight() const = 0;

        virtual void SetCallback(const EventCallbackFn& callback) = 0;

        virtual void SetVSync(bool enabled) = 0;
        virtual bool IsVSync() const = 0;

        static std::unique_ptr<Window> Create(const WindowProps& props = WindowProps());
    };
}
