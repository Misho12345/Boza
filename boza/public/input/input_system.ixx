module;

#include "api.hpp"

export module boza.input:input_system;

import std;
import boza.common;
import boza.ecs;

import :keys;
import :cursor_state;
import :input_capture;
import :key_state;

namespace boza
{
    export class BOZA_API Input final
    {
    public:
        Input() = delete;

        static bool is_pressed(Key key);
        static bool is_held(Key key);

        static void reset_cursor_tracking();
    };

    export struct BOZA_API InputSystem
    {
        struct InputFrameData final
        {
            flat_map<Key, KeyState> key_states{};
            std::vector<Key> keys_pressed{};
            std::vector<Key> keys_released{};
            std::vector<Key> keys_double_clicked{};
            glm::vec2 mouse_delta{ 0.0f, 0.0f };
            glm::vec2 scroll_delta{ 0.0f, 0.0f };
        };

        static inline InputFrameData frame{};

        static inline glm::vec2 last_cursor_pos{ 0.0f, 0.0f };
        static inline bool first_cursor_move{ true };

        struct Begin final : EngineBeginStage<Begin>
        {
            static void execute();
        };

        struct Input final : EngineUpdateStage<Input>
        {
            static void execute();
        };

        struct Update final : EngineUpdateStage<
                                  Update,
                                  RunAfter<Input>,
                                  With<InputCapture>
                              >
        {
            static void execute(InputCapture& capture);
        };

        struct Destroy final : EngineDestroyStage<Destroy>
        {
            static void execute();
        };

    private:
        static void process_press_events(InputCapture& capture, Key key, const flat_map<Key, KeyState>& states);
        static void process_release_events(InputCapture& capture, Key key, const flat_map<Key, KeyState>& states);
        static void process_double_click_events(InputCapture& capture, Key key, const flat_map<Key, KeyState>& states);
        static void process_hold_events(InputCapture& capture, const flat_map<Key, KeyState>& states);
        static void process_mouse_move(InputCapture& capture, glm::vec2 delta);
        static void process_mouse_scroll(InputCapture& capture, glm::vec2 offset);

        friend class boza::Input;
    };
}
