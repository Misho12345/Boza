#pragma once

#ifdef BOZA_OPENGL_ENABLED
#define BOZA_IF_OPENGL(...) __VA_ARGS__
#else
#define BOZA_IF_OPENGL(...)
#endif

#ifdef BOZA_VULKAN_ENABLED
#define BOZA_IF_VULKAN(...) __VA_ARGS__
#else
#define BOZA_IF_VULKAN(...)
#endif

#ifdef BOZA_METAL_ENABLED
#define BOZA_IF_METAL(...) __VA_ARGS__
#else
#define BOZA_IF_METAL(...)
#endif

#ifdef BOZA_DX11_ENABLED
#define BOZA_IF_DX11(...) __VA_ARGS__
#else
#define BOZA_IF_DX11(...)
#endif

#ifdef BOZA_DX12_ENABLED
#define BOZA_IF_DX12(...) __VA_ARGS__
#else
#define BOZA_IF_DX12(...)
#endif