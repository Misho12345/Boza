export module boza.input:cursor_state;

import std;

export namespace boza
{
    enum class CursorState : std::uint8_t
    {
        Normal,
        Hidden,
        Locked,
        HiddenLocked
    };
}
