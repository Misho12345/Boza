export module boza.rhi:buffer_layout;

import std;
import boza.common;

export namespace boza::rhi
{
    enum class BufferLayoutType : std::uint8_t
    {
        Std140,
        Std430,
        PushConstant
    };

    enum class ShaderValueType : std::uint8_t
    {
        Float,
        Int, Uint,
        Bool,
        Vec2, Vec3, Vec4,
        IVec2, IVec3, IVec4,
        UVec2, UVec3, UVec4,
        Mat2, Mat3, Mat4
    };

    struct BufferLayoutRules
    {
        virtual ~BufferLayoutRules() = default;

        [[nodiscard]] virtual std::size_t get_alignment(ShaderValueType type) const = 0;
        [[nodiscard]] virtual std::size_t get_size(ShaderValueType type) const = 0;
        [[nodiscard]] virtual std::size_t get_array_stride(ShaderValueType type) const = 0;
        [[nodiscard]] virtual std::size_t align_offset(std::size_t offset, ShaderValueType type) const = 0;
    };

    class Std140LayoutRules final : public BufferLayoutRules
    {
    public:
        [[nodiscard]]
        std::size_t get_alignment(const ShaderValueType type) const override
        {
            switch (type)
            {
                case ShaderValueType::Float:
                case ShaderValueType::Int:
                case ShaderValueType::Uint:
                case ShaderValueType::Bool: return 4;
                case ShaderValueType::Vec2:
                case ShaderValueType::IVec2:
                case ShaderValueType::UVec2: return 8;
                case ShaderValueType::Vec3:
                case ShaderValueType::IVec3:
                case ShaderValueType::UVec3:
                case ShaderValueType::Vec4:
                case ShaderValueType::IVec4:
                case ShaderValueType::UVec4:
                case ShaderValueType::Mat2:
                case ShaderValueType::Mat3:
                case ShaderValueType::Mat4: return 16;
            }

            std::unreachable();
        }

        [[nodiscard]]
        std::size_t get_size(const ShaderValueType type) const override
        {
            switch (type)
            {
                case ShaderValueType::Float:
                case ShaderValueType::Int:
                case ShaderValueType::Uint:
                case ShaderValueType::Bool: return 4;
                case ShaderValueType::Vec2:
                case ShaderValueType::IVec2:
                case ShaderValueType::UVec2: return 8;
                case ShaderValueType::Vec3:
                case ShaderValueType::IVec3:
                case ShaderValueType::UVec3:
                case ShaderValueType::Vec4:
                case ShaderValueType::IVec4:
                case ShaderValueType::UVec4: return 16;
                case ShaderValueType::Mat2: return 32;
                case ShaderValueType::Mat3: return 48;
                case ShaderValueType::Mat4: return 64;
            }

            std::unreachable();
        }

        [[nodiscard]]
        std::size_t get_array_stride(const ShaderValueType type) const override
        {
            const std::size_t base_size = get_size(type);
            return (base_size + 15) / 16 * 16;
        }

        [[nodiscard]]
        std::size_t align_offset(const std::size_t offset, const ShaderValueType type) const override
        {
            const std::size_t alignment = get_alignment(type);
            return (offset + alignment - 1) / alignment * alignment;
        }
    };

    class Std430LayoutRules final : public BufferLayoutRules
    {
    public:
        [[nodiscard]]
        std::size_t get_alignment(const ShaderValueType type) const override
        {
            switch (type)
            {
                case ShaderValueType::Float:
                case ShaderValueType::Int:
                case ShaderValueType::Uint:
                case ShaderValueType::Bool: return 4;
                case ShaderValueType::Vec2:
                case ShaderValueType::IVec2:
                case ShaderValueType::UVec2: return 8;
                case ShaderValueType::Vec3:
                case ShaderValueType::IVec3:
                case ShaderValueType::UVec3:
                case ShaderValueType::Vec4:
                case ShaderValueType::IVec4:
                case ShaderValueType::UVec4:
                case ShaderValueType::Mat3:
                case ShaderValueType::Mat4: return 16;
                case ShaderValueType::Mat2: return 8;
            }

            std::unreachable();
        }

        [[nodiscard]]
        std::size_t get_size(const ShaderValueType type) const override
        {
            switch (type)
            {
                case ShaderValueType::Float:
                case ShaderValueType::Int:
                case ShaderValueType::Uint:
                case ShaderValueType::Bool: return 4;
                case ShaderValueType::Vec2:
                case ShaderValueType::IVec2:
                case ShaderValueType::UVec2: return 8;
                case ShaderValueType::Vec3:
                case ShaderValueType::IVec3:
                case ShaderValueType::UVec3: return 12;
                case ShaderValueType::Vec4:
                case ShaderValueType::IVec4:
                case ShaderValueType::UVec4:
                case ShaderValueType::Mat2: return 16;
                case ShaderValueType::Mat3: return 48;
                case ShaderValueType::Mat4: return 64;
            }

            std::unreachable();
        }

        [[nodiscard]]
        std::size_t get_array_stride(const ShaderValueType type) const override
        {
            const std::size_t size      = get_size(type);
            const std::size_t alignment = get_alignment(type);
            return (size + alignment - 1) / alignment * alignment;
        }

        [[nodiscard]]
        std::size_t align_offset(const std::size_t offset, const ShaderValueType type) const override
        {
            const std::size_t alignment = get_alignment(type);
            return (offset + alignment - 1) / alignment * alignment;
        }
    };

    class BufferWriter final
    {
    public:
        explicit BufferWriter(const BufferLayoutType layout_type)
            : layout_type_(layout_type)
        {
            layout_type == BufferLayoutType::Std140
                ? rules_ = std::make_unique<Std140LayoutRules>()
                : rules_ = std::make_unique<Std430LayoutRules>();
        }

        void reset()
        {
            data_.clear();
            current_offset_ = 0;
        }

        template <typename T>
        void write(const T& value)
        {
            const ShaderValueType type = get_shader_type<T>();
            const std::size_t aligned_offset = rules_->align_offset(current_offset_, type);
            const std::size_t size = rules_->get_size(type);

            if (aligned_offset + size > data_.size()) { data_.resize(aligned_offset + size, 0); }

            std::memcpy(data_.data() + aligned_offset, &value, sizeof(T));
            current_offset_ = aligned_offset + size;
        }

        void write_at(const std::size_t offset, const void* value, const std::size_t size)
        {
            if (offset + size > data_.size()) data_.resize(offset + size, 0);
            std::memcpy(data_.data() + offset, value, size);
        }

        [[nodiscard]] const std::uint8_t* data() const { return data_.data(); }
        [[nodiscard]] std::size_t size() const { return data_.size(); }
        [[nodiscard]] std::size_t current_offset() const { return current_offset_; }

        void reserve(const std::size_t size) { data_.reserve(size); }
        void resize(const std::size_t size) { data_.resize(size, 0); }

    private:
        template <typename T>
        static constexpr ShaderValueType get_shader_type()
        {
            if constexpr (std::same_as<T, float>) return ShaderValueType::Float;
            else if constexpr (std::same_as<T, std::int32_t>) return ShaderValueType::Int;
            else if constexpr (std::same_as<T, std::uint32_t>) return ShaderValueType::Uint;
            else if constexpr (std::same_as<T, bool>) return ShaderValueType::Bool;
            else if constexpr (std::same_as<T, glm::vec2>) return ShaderValueType::Vec2;
            else if constexpr (std::same_as<T, glm::vec3>) return ShaderValueType::Vec3;
            else if constexpr (std::same_as<T, glm::vec4>) return ShaderValueType::Vec4;
            else if constexpr (std::same_as<T, glm::ivec2>) return ShaderValueType::IVec2;
            else if constexpr (std::same_as<T, glm::ivec3>) return ShaderValueType::IVec3;
            else if constexpr (std::same_as<T, glm::ivec4>) return ShaderValueType::IVec4;
            else if constexpr (std::same_as<T, glm::uvec2>) return ShaderValueType::UVec2;
            else if constexpr (std::same_as<T, glm::uvec3>) return ShaderValueType::UVec3;
            else if constexpr (std::same_as<T, glm::uvec4>) return ShaderValueType::UVec4;
            else if constexpr (std::same_as<T, glm::mat2>) return ShaderValueType::Mat2;
            else if constexpr (std::same_as<T, glm::mat3>) return ShaderValueType::Mat3;
            else if constexpr (std::same_as<T, glm::mat4>) return ShaderValueType::Mat4;
            else static_assert(false, "Unsupported shader type in BufferWriter::write");
        }

        BufferLayoutType layout_type_;
        std::unique_ptr<BufferLayoutRules> rules_;
        std::vector<std::uint8_t> data_;
        std::size_t current_offset_{ 0 };
    };

    [[nodiscard]]
    inline const BufferLayoutRules& get_layout_rules(const BufferLayoutType type)
    {
        static Std140LayoutRules std140;
        static Std430LayoutRules std430;

        switch (type)
        {
            case BufferLayoutType::Std140: return std140;
            case BufferLayoutType::PushConstant:
            case BufferLayoutType::Std430: return std430;
        }

        std::unreachable();
    }
}
