export module game.player;

import std;
import boza;
using namespace boza;

export namespace game::player
{
    struct PlayerTag final {};
    GameObject create(const GameObject& terrain_world);
}
