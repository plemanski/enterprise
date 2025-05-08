#pragma once
#include "Application.h"

#ifdef EE_PLATFORM_WINDOWS

extern Enterprise::Application* Enterprise::CreateApplication();

int main(int argc, char** argv)
{
    Enterprise::Log::Init();

    auto app = Enterprise::CreateApplication();
    app -> Run();
    delete app;
}

#endif