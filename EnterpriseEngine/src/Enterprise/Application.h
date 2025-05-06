#pragma once

#include "Core.h"

namespace Enterprise
{
    class ENTERPRISE_API Application
    {
    public:
        Application();
        virtual ~Application();

        void Run();
    };

    Application* CreateApplication();

}
