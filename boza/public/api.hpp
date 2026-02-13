#pragma once

// #ifdef _WIN32
//     #ifdef BOZAENGINE_EXPORTS
//         #define BOZA_API __declspec(dllexport)
//     #else
//         #define BOZA_API __declspec(dllimport)
//     #endif
// #else
//     #ifdef BOZAENGINE_EXPORTS
//         #define BOZA_API __attribute__((visibility("default")))
//     #else
//         #define BOZA_API
//     #endif
// #endif

// Temporarily disable dll import/export because there are issues with using the engine as a DLL
#ifdef _WIN32
    #ifdef BOZAENGINE_EXPORTS
        #define BOZA_API
    #else
        #define BOZA_API
    #endif
#else
    #ifdef BOZAENGINE_EXPORTS
        #define BOZA_API
    #else
        #define BOZA_API
    #endif
#endif
