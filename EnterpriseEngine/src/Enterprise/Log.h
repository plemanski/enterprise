#pragma once

#include <memory>

#include "Core.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace Enterprise
{
    class ENTERPRISE_API Log
    {
    public:
        static void Init();

        inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
        inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

    private:
        static std::shared_ptr<spdlog::logger> s_CoreLogger;
        static std::shared_ptr<spdlog::logger> s_ClientLogger;
    };    
}

//Core Log Macros
#define EE_CORE_TRACE(...) ::Enterprise::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define EE_CORE_INFO(...)  ::Enterprise::Log::GetCoreLogger()->info(__VA_ARGS__)
#define EE_CORE_WARN(...)  ::Enterprise::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define EE_CORE_ERROR(...) ::Enterprise::Log::GetCoreLogger()->error(__VA_ARGS__)

// Client Log Macros
#define EE_TRACE(...) ::Enterprise::Log::GetClientLogger()->trace(__VA_ARGS__)
#define EE_INFO(...)  ::Enterprise::Log::GetClientLogger()->info(__VA_ARGS__)
#define EE_WARN(...)  ::Enterprise::Log::GetClientLogger()->warn(__VA_ARGS__)
#define EE_ERROR(...) ::Enterprise::Log::GetClientLogger()->error(__VA_ARGS__)
