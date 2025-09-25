#pragma once

#ifdef BOZA_OPENAL_ENABLED
#define BOZA_IF_OPENAL(...) __VA_ARGS__
#else
#define BOZA_IF_OPENAL(...)
#endif

#ifdef BOZA_XAUDIO2_ENABLED
#define BOZA_IF_XAUDIO2(...) __VA_ARGS__
#else
#define BOZA_IF_XAUDIO2(...)
#endif

#ifdef BOZA_COREAUDIO_ENABLED
#define BOZA_IF_COREAUDIO(...) __VA_ARGS__
#else
#define BOZA_IF_COREAUDIO(...)
#endif

namespace boza
{
    enum class AudioApi
    {
        BOZA_IF_OPENAL(OpenAL,)
        BOZA_IF_XAUDIO2(XAudio2,)
        BOZA_IF_COREAUDIO(CoreAudio)
    };
}
