#pragma once
#include "boza/core/Mesh.hpp"
#include <memory>

namespace boza::Primitive
{
    inline std::shared_ptr<Mesh> create_quad()
    {
        std::vector<Vertex> vertices =
            {
            { { -0.5f,  0.0f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 0.0f } },
            { {  0.5f,  0.0f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 0.0f } },
            { {  0.5f,  0.0f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f } },
            { { -0.5f,  0.0f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 1.0f } },
        };

        std::vector<uint32_t> indices = { 0, 1, 2, 0, 2, 3 };

        return std::make_shared<Mesh>(std::move(vertices), std::move(indices));
    }

    inline std::shared_ptr<Mesh> create_cube()
    {
        std::vector<Vertex> vertices = {
            // Front face (Z+)
            { { -0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 0.0f } },
            { {  0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 0.0f } },
            { {  0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f } },
            { { -0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 1.0f } },

            // Back face (Z-)
            { {  0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 0.0f } },
            { { -0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 0.0f } },
            { { -0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f } },
            { {  0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 1.0f } },

            // Top face (Y+)
            { { -0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 0.0f } },
            { {  0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 0.0f } },
            { {  0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f } },
            { { -0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 1.0f } },

            // Bottom face (Y-)
            { { -0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 0.0f } },
            { {  0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 0.0f } },
            { {  0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 1.0f } },
            { { -0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 1.0f } },

            // Right face (X+)
            { {  0.5f, -0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } },
            { {  0.5f, -0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f } },
            { {  0.5f,  0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } },
            { {  0.5f,  0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } },

            // Left face (X-)
            { { -0.5f, -0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } },
            { { -0.5f, -0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f } },
            { { -0.5f,  0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } },
            { { -0.5f,  0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } }
        };

        std::vector<uint32_t> indices = {
            0, 1, 2,   0, 2, 3,     // Front (CCW from outside)
            4, 5, 6,   4, 6, 7,     // Back (CCW from outside)
            8, 9, 10,  8, 10, 11,   // Top (CCW from outside)
            12, 13, 14, 12, 14, 15, // Bottom (CCW from outside)
            16, 17, 18, 16, 18, 19, // Right (CCW from outside)
            20, 21, 22, 20, 22, 23  // Left (CCW from outside)
        };

        return std::make_shared<Mesh>(std::move(vertices), std::move(indices));
    }
}
