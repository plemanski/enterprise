#pragma once

#ifdef EE_PLATFORM_WINDOWS
    #ifdef EE_BUILD_DLL
        #define ENTERPRISE_API __declspec(dllexport)
    #else
        #define ENTERPRISE_API __declspec(dllimport)
    #endif
#else
    #error Enterprise only supports Windows

#endif

