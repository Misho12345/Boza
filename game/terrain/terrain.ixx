export module game.terrain;

export import :common;

export namespace game::terrain
{
    GameObject create(std::string_view name, const Settings& settings, bool show_chunk_bounds = true);

    bool ready(const GameObject& terrain_world);

    glm::vec3 world_size(const GameObject& terrain_world);
    glm::vec3 world_center(const GameObject& terrain_world);
    float max_brush_radius(const GameObject& terrain_world);

    bool intersects_world_sphere(const GameObject& terrain_world, const glm::vec3& center, float radius);
    void queue_edit(GameObject& terrain_world, const EditCommand& edit);

    void set_chunk_bounds_visible(GameObject& terrain_world, bool visible);
    bool chunk_bounds_visible(const GameObject& terrain_world);
}
