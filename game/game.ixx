export module game;

import boza;
using namespace boza;

import game.config;
import game.terrain;
import game.player;

namespace game
{
    class Game final : public App
    {
    protected:
        void setup() override
        {
            Scene::main = Scene::create("Game");
            create_cube_mesh();

            const auto terrain_world = terrain::create(
                "Terrain",
                {
                    .chunk_size = config::terrain_chunk_size,
                    .chunk_counts = config::terrain_chunk_counts,
                });

            player::create(terrain_world);
        }

        static void create_cube_mesh()
        {
            Mesh::create_obj("cube", "primitives/cube.obj");
        }
    };
}

export using game::Game;
