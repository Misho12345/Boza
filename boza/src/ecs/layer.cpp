module boza.ecs;

import boza.detail;

namespace boza
{
    using detail::LayerManager;

    Layer::Layer(const std::string& name) : mask_{ LayerManager::instance().layer_mask(name) } {}

    Layer::Layer(const Layer& other) { mask_ = other.mask_;}
    Layer& Layer::operator=(const Layer& other)
    {
        if (&other != this) mask_ = other.mask_;
        return *this;
    }

    Layer& Layer::operator=(const uint32_t mask_value)
    {
        mask_ = mask_value;
        return *this;
    }

    Layer& Layer::operator=(const std::string& layer_name)
    {
        mask_ = LayerManager::instance().layer_mask(layer_name);
        return *this;
    }

    Layer Layer::operator|(const Layer& other) const { return Layer(mask_ | other.mask_); }
    Layer Layer::operator&(const Layer& other) const { return Layer(mask_ & other.mask_); }
    Layer Layer::operator^(const Layer& other) const { return Layer(mask_ ^ other.mask_); }
    Layer Layer::operator~() const { return Layer(~mask_); }

    Layer& Layer::operator|=(const Layer& other)
    {
        mask_ |= other.mask_;
        return *this;
    }

    Layer& Layer::operator&=(const Layer& other)
    {
        mask_ &= other.mask_;
        return *this;
    }

    Layer& Layer::operator^=(const Layer& other)
    {
        mask_ ^= other.mask_;
        return *this;
    }


    bool Layer::contains(const Layer& other) const { return (mask_ & other.mask_) != 0; }

    bool Layer::contains(const std::string& layer_name) const
    {
        const uint32_t layer_mask = LayerManager::instance().layer_mask(layer_name);
        return (mask_ & layer_mask) != 0;
    }

    bool Layer::operator==(const Layer& other) const { return mask_ == other.mask_; }
    bool Layer::operator!=(const Layer& other) const { return mask_ != other.mask_; }
    bool Layer::operator!=(const std::string& layer_name) const { return !(*this == layer_name); }

    bool Layer::operator==(const std::string& layer_name) const
    {
        return mask_ == LayerManager::instance().layer_mask(layer_name);
    }

    std::uint32_t Layer::get_mask() const { return mask_; }
    const std::string& Layer::get_name() const { return LayerManager::instance().layer_name(mask_); }
}
