module boza.ecs;
import boza.detail;

namespace boza
{
    using detail::TagManager;

    Tag::Tag(const std::string& name) : id_{ TagManager::instance().tag_id(name) } {}

    Tag::Tag(const Tag& other) { id_ = other.id_; }
    Tag& Tag::operator=(const Tag& other)
    {
        if (&other != this) id_ = other.id_;
        return *this;
    }

    Tag& Tag::operator=(const std::string& tag_name)
    {
        id_ = TagManager::instance().tag_id(tag_name);
        return *this;
    }

    Tag& Tag::operator=(const uint32_t tag_id)
    {
        id_ = tag_id;
        return *this;
    }

    bool Tag::operator==(const Tag& other) const { return id_ == other.id_; }
    bool Tag::operator!=(const Tag& other) const { return id_ != other.id_; }
    bool Tag::operator==(const std::string& tag_name) const { return id_ == TagManager::instance().tag_id(tag_name); }
    bool Tag::operator!=(const std::string& tag_name) const { return !(*this == tag_name); }

    std::uint32_t Tag::get_id() const { return id_; }
    const std::string& Tag::get_name() const { return TagManager::instance().tag_name(id_); }
}