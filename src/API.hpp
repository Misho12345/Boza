#pragma once

#ifdef _WIN32
    #ifdef BOZAENGINE_EXPORTS
        #define BOZA_API __declspec(dllexport)
    #else
        #define BOZA_API __declspec(dllimport)
    #endif
#else
    #define BOZA_API
#endif
