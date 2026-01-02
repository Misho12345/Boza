export module boza.detail:audio_api;

import std;
// TODO: add that in ahi/ when ahi is implemented

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

export namespace boza::detail
{
    enum class AudioApi : std::uint8_t
    {
        BOZA_IF_OPENAL(OpenAL,)
        BOZA_IF_XAUDIO2(XAudio2,)
        BOZA_IF_COREAUDIO(CoreAudio)
    };

    // TODO: add priority array like in graphics_api.ixx
}
