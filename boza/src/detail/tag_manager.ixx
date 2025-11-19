export module boza.detail:tag_manager;
import std;

export namespace boza::detail
{
    class TagManager final
    {
    public:
        static TagManager& instance();

        void register_tag(const std::string& name, std::uint32_t id);

        [[nodiscard]] std::uint32_t tag_id(const std::string& name) const;
        [[nodiscard]] const std::string& tag_name(std::uint32_t id) const;

        [[nodiscard]] bool has_tag(const std::string& name) const;
        void clear();

    private:
        TagManager() = default;

        std::unordered_map<std::string, std::uint32_t> name_to_id_{};
        std::unordered_map<std::uint32_t, std::string> id_to_name_{};
    };
}