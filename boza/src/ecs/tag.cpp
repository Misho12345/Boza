module boza.ecs;
import boza.detail;

namespace boza
{
    using detail::TagManager;

    Tag::Tag(const std::string& name) : id_(TagManager::instance().tag_id(name)) {}

    Tag& Tag::operator=(const std::string& _name)
    {
        id_ = TagManager::instance().tag_id(_name);
        return *this;
    }

    Tag& Tag::operator=(const uint32_t _id)
    {
        id_ = _id;
        return *this;
    }

    bool Tag::operator==(const Tag& other) const { return id_ == other.id_; }
    bool Tag::operator!=(const Tag& other) const { return id_ != other.id_; }
    bool Tag::operator==(const std::string& _name) const { return id_ == TagManager::instance().tag_id(_name); }
    bool Tag::operator!=(const std::string& _name) const { return !(*this == _name); }

    std::uint32_t Tag::get_id() const { return id_; }
    std::string Tag::get_name() const { return TagManager::instance().tag_name(id_); }
}