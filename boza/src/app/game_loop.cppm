export module boza.app:game_loop;

namespace boza::app
{
    export struct GameLoopConfig final
    {
        float target_fps{ 60.0f };
        float physics_update_rate{ 60.0f };
    };

    export class GameLoop final
    {
    public:
        void init(const GameLoopConfig& config);
        [[nodiscard]] bool run_engine_begin_stages() const;
        void run() const;

        [[nodiscard]]
        float get_target_fps() const;
        void set_target_fps(float fps);

    private:
        GameLoopConfig config_{};
    };
}
