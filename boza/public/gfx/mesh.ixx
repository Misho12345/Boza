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

    struct Mesh final
    {
        struct BoundingSphere final
        {
            glm::vec3 center{ 0.0f };
            float radius{ 0.0f };
        };

        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> indices;
        std::string name;
        BoundingSphere bounds{};

        void recalculate_bounds();

        /// Registers a mesh with the given name
        /// @param name The name to register the mesh under
        /// @param mesh The mesh to register
        static void register_mesh(std::string_view name, Mesh mesh);

        /// Gets a mesh by name
        /// @param name The name of the mesh
        /// @return Reference to the mesh (throws if not found)
        static Mesh& get(std::string_view name);

        /// Tries to get a mesh by name
        /// @param name The name of the mesh
        /// @return Pointer to the mesh, or nullptr if not found
        static Mesh* try_get(std::string_view name);

        /// Checks if a mesh is registered
        /// @param name The name of the mesh
        /// @return true if the mesh exists
        static bool exists(std::string_view name);

    private:
        static inline node_map<std::string, Mesh> registry_;
    };
}
