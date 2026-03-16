export module boza.gfx:mesh;

import std;
import boza.common;

export namespace boza
{
    struct Vertex final
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 tex_coord;
    };

    struct BoundingSphere final
    {
        glm::vec3 center{ 0.0f };
        float     radius{ 0.0f };
    };

    class Mesh final
    {
        const std::vector<Vertex>& get_vertices() const { return vertices_; }
        const std::vector<std::uint32_t>& get_indices() const { return indices_; }
        const std::string& get_name() const { return name_; }
        const BoundingSphere& get_bounds() const { return bounds_; }

    public:
        Mesh(Mesh&& other) noexcept;
        Mesh& operator=(Mesh&& other) noexcept;
        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        static Mesh& create(
            std::string_view           name,
            std::vector<Vertex>        vertices,
            std::vector<std::uint32_t> indices);

        static Mesh& replace(
            std::string_view           name,
            std::vector<Vertex>        vertices,
            std::vector<std::uint32_t> indices);

        static Mesh& get(std::string_view name);
        static Mesh* try_get(std::string_view name);

        static bool exists(std::string_view name);
        static bool exists(const Mesh* ptr);

        [[nodiscard]] std::uint64_t revision() const { return revision_; }

        [[msvc::no_unique_address]] Property<Mesh, &Mesh::get_name> name{ this };
        [[msvc::no_unique_address]] Property<Mesh, &Mesh::get_vertices> vertices{ this };
        [[msvc::no_unique_address]] Property<Mesh, &Mesh::get_indices> indices{ this };
        [[msvc::no_unique_address]] Property<Mesh, &Mesh::get_bounds> bounds{ this };

    private:
        Mesh(const std::string_view    mesh_name,
            std::vector<Vertex>        mesh_vertices,
            std::vector<std::uint32_t> mesh_indices)
            : name_{ mesh_name },
              vertices_{ std::move(mesh_vertices) },
              indices_{ std::move(mesh_indices) } {}

        void recalculate_bounds();

        std::string                name_;
        std::vector<Vertex>        vertices_;
        std::vector<std::uint32_t> indices_;
        BoundingSphere             bounds_{};
        std::uint64_t              revision_{ 1 };

        static inline node_map<std::string, Mesh> registry_;
        static inline flat_set<const Mesh*>       valid_pointers_;
    };
}
