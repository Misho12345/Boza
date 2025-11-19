module boza.ecs;

namespace boza
{
    bool Component::get_enabled() const { return enabled_; }
    void Component::set_enabled(const bool value) { enabled_ = value; }
}
