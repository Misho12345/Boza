#include "boza/core/Tag.hpp"
#include "boza/core/TagManager.hpp"

namespace boza
{
    Tag::Tag(const std::string& name) : id_(TagManager::instance().get_tag_id(name)) {}

    Tag& Tag::operator=(const std::string& _name)
    {
        id_ = TagManager::instance().get_tag_id(_name);
        return *this;
    }

    Tag& Tag::operator=(const uint32_t _id)
    {
        id_ = _id;
        return *this;
    }

    bool Tag::operator==(const Tag& other) const { return id_ == other.id_; }
    bool Tag::operator!=(const Tag& other) const { return id_ != other.id_; }
    bool Tag::operator==(const std::string& _name) const { return id_ == TagManager::instance().get_tag_id(_name); }
    bool Tag::operator!=(const std::string& _name) const { return !(*this == _name); }

    std::string Tag::get_name() const { return TagManager::instance().get_tag_name(id_); }
}
