export module instancing_example:grass;

import std;
import boza;
using namespace boza;

export Mesh create_grass_blade_mesh()
{
    Mesh mesh{};

    constexpr int   num_segments    = 5;      // Number of rectangular segments
    constexpr float base_width      = 0.1f;   // Width at the bottom
    constexpr float tip_width_ratio = 0.4f;   // Top segments are 40% of base width
    constexpr float total_height    = 2.2f;   // Total blade height
    constexpr float curve_amount    = 0.15f;  // How much the blade curves backward

    constexpr float segment_height = total_height / static_cast<float>(num_segments + 1); // +1 for triangle tip

    mesh.vertices.reserve((num_segments + 1) * 2 + 1); // Each segment has 2 verts, plus tip vertex
    mesh.indices.reserve(num_segments * 6 + 3);        // 2 triangles per segment, 1 for tip

    // Build the blade from bottom to top
    for (int seg = 0; seg <= num_segments; ++seg)
    {
        const float t = static_cast<float>(seg) / static_cast<float>(num_segments);
        const float y = segment_height * static_cast<float>(seg);

        // Calculate width taper - thinner at top
        float width_scale = 1.0f;
        if (seg >= num_segments - 1) // Top 2 segments are thinner
        {
            const float top_t = static_cast<float>(seg - (num_segments - 2)) / 2.0f;
            width_scale       = glm::mix(1.0f, tip_width_ratio, top_t);
        }
        const float half_width = base_width * width_scale;

        // Calculate backward curve (quadratic easing for natural droop)
        const float curve_offset = -curve_amount * t * t;

        // Calculate normal (pointing toward viewer, adjusted for curve)
        const glm::vec3 tangent = normalize(glm::vec3(0.0f, 1.0f, curve_offset * 2.0f));
        const glm::vec3 normal  = normalize(glm::vec3(0.0f, -tangent.z, tangent.y));

        // UV coordinates
        const float v_coord = t;

        // Left and right vertices for this segment
        mesh.vertices.push_back({
            .position  = glm::vec3(-half_width, y, curve_offset),
            .normal    = normal,
            .tex_coord = glm::vec2(0.0f, v_coord)
        });

        mesh.vertices.push_back({
            .position  = glm::vec3(half_width, y, curve_offset),
            .normal    = normal,
            .tex_coord = glm::vec2(1.0f, v_coord)
        });
    }

    // Add the tip vertex (triangle at the very top)
    constexpr float tip_y     = total_height;
    constexpr float tip_curve = -curve_amount; // Maximum curve at the tip
    mesh.vertices.push_back({
        .position  = glm::vec3(0.0f, tip_y, tip_curve),
        .normal    = glm::vec3(0.0f, 0.0f, 1.0f),
        .tex_coord = glm::vec2(0.5f, 1.0f)
    });

    // Build indices for rectangular segments
    for (int seg = 0; seg < num_segments; ++seg)
    {
        const std::uint32_t base_idx = static_cast<std::uint32_t>(seg * 2);
        const std::uint32_t i0       = base_idx;     // Bottom left
        const std::uint32_t i1       = base_idx + 1; // Bottom right
        const std::uint32_t i2       = base_idx + 2; // Top left
        const std::uint32_t i3       = base_idx + 3; // Top right

        // Two triangles forming a quad
        mesh.indices.insert(
            mesh.indices.end(),
            {
                i0, i1, i2, // First triangle
                i1, i3, i2  // Second triangle
            });
    }

    // Add the tip triangle
    const std::uint32_t     tip_idx    = static_cast<std::uint32_t>(mesh.vertices.size() - 1);
    constexpr std::uint32_t last_left  = num_segments * 2;
    constexpr std::uint32_t last_right = last_left + 1;

    mesh.indices.insert(
        mesh.indices.end(), {
            last_left, last_right, tip_idx
        });

    return mesh;
}
